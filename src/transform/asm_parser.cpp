#include "asm_parser.hpp"
#include "common/asm_ast.hpp"
#include "common/tac.hpp"
#include "util/assert.hpp"
#include <unordered_map>

using tac::Tac;

using namespace asm_parser;
using namespace asm_ast;

AsmParser::AsmParser(const Tac& tac_in, const tac::TacContainer& tac_container)
    : tac_top{tac_in}, tac_container{tac_container}, instructions{},
      operands{} {}

ParseResult AsmParser::parse() {
  Program p = program(tac_top);

  return {p, instructions, operands};
}

Program AsmParser::program(const Tac& tac) {
  auto& prog = tac.get<tac::Tag::PROGRAM>();

  auto fn = function(prog.function.from(tac_container));

  uint64_t stack_size = fix_pseudos(fn);
  add_scaffolding(fn, stack_size);
  fix_operands(fn);

  // Assume it is a function for now
  return Program{fn};
}

Function AsmParser::function(const Tac& tac) {
  auto& fn = tac.get<tac::Tag::FUNCTION>();

  Function ret{fn.name, {}};
  for (const auto& inst : tac_container.iterate_content(fn.instructions)) {
    instruction(inst, ret.instructions);
  }

  return ret;
}

uint64_t AsmParser::fix_pseudos(Function& fn) {
  uint64_t total_stack = 0;
  std::unordered_map<std::string, OperandIndex> addresses{};

  auto introduce_pseudo = [this, &total_stack,
                           &addresses](const OperandPseudo& pseudo) {
    if (addresses.count(pseudo.pseudo_value) == 0) {
      addresses[pseudo.pseudo_value] =
          add_operand(OperandStackAddress{(signed)total_stack});
      total_stack += 8;
      return addresses[pseudo.pseudo_value];
    } else {
      return addresses[pseudo.pseudo_value];
    }
  };

  for (auto& instr : instructions.iterate_content(fn.instructions)) {
    switch (instr.get_tag()) {
      using enum InstructionTag;
    case LOAD: {
      auto& ld = instr.get<LOAD>();
      auto& source = ld.source.from(operands);
      auto& dest = ld.destination.from(operands);
      if (source.is_a(OperandTag::PSEUDO)) {
        ld.source = introduce_pseudo(source.get<OperandTag::PSEUDO>());
      }
      if (dest.is_a(OperandTag::PSEUDO)) {
        ld.destination = introduce_pseudo(dest.get<OperandTag::PSEUDO>());
      }
    } break;
    case STORE: {
      auto& st = instr.get<STORE>();
      auto& source = st.source.from(operands);
      auto& dest = st.destination.from(operands);
      if (source.is_a(OperandTag::PSEUDO)) {
        st.source = introduce_pseudo(source.get<OperandTag::PSEUDO>());
      }
      if (dest.is_a(OperandTag::PSEUDO)) {
        st.destination = introduce_pseudo(dest.get<OperandTag::PSEUDO>());
      }
    } break;
    case UNARY: {
      auto& unary = instr.get<UNARY>();
      auto& arg = unary.arg.from(operands);
      auto& dest = unary.destination.from(operands);
      if (arg.is_a(OperandTag::PSEUDO)) {
        unary.arg = introduce_pseudo(arg.get<OperandTag::PSEUDO>());
      }
      if (dest.is_a(OperandTag::PSEUDO)) {
        unary.destination = introduce_pseudo(dest.get<OperandTag::PSEUDO>());
      }
    } break;
    case ALLOCA: [[fallthrough]];
    case RET: [[fallthrough]];
    case LAST: break;
    }
  }

  return total_stack + DEFAULT_STACK;
}

void AsmParser::add_scaffolding(Function& fn, uint64_t stack_size) {
  auto& instrs = fn.instructions;
  instrs.insert(instrs.begin(),
                add_instruction(InstructionAllocateStack{stack_size}));

  // This is wasteful, we're going over it twice
  // That's okay for now though
  for (std::size_t i = 0; i < instrs.size(); ++i) {
    auto& instr = instrs[i].from(instructions);
    if (instr.is_a(InstructionTag::RET)) {
      instrs.insert(instrs.begin() + i,
                    add_instruction(InstructionAllocateStack{-stack_size}));
      ++i;
    }
  }
}

void AsmParser::fix_operands(Function& fn) {
  auto& instrs = fn.instructions;
  for (std::size_t i = 0; i < instrs.size(); ++i) {
    auto& instr = instrs[i].from(instructions);

    switch (instr.get_tag()) {
      using enum InstructionTag;
    case LOAD: {
      auto& ld = instr.get<LOAD>();
      auto& dest = ld.destination.from(operands);
      if (dest.is_a(OperandTag::STACK_ADDR)) {
        OperandIndex dest = ld.destination;
        ld.destination = add_operand(OperandRegister{Register::T0});
        instrs.insert(instrs.begin() + (i + 1),
                      add_instruction(InstructionStore{ld.destination, dest}));
        ++i; // skip over just created
      }
    } break;
    case STORE: {
      auto& st = instr.get<STORE>();
      auto& src = st.source.from(operands);
      if (src.is_a(OperandTag::STACK_ADDR)) {
        OperandIndex src = st.source;
        st.source = add_operand(OperandRegister{Register::T0});
        instrs.insert(instrs.begin() + i,
                      add_instruction(InstructionLoad{src, st.source}));
        ++i; // skip over just created
      }
    } break;
    case UNARY: {
      {
        auto& un = instr.get<UNARY>();
        auto& arg = un.arg.from(operands);

        if (arg.is_a(OperandTag::STACK_ADDR) ||
            arg.is_a(OperandTag::IMMEDIATE)) {
          OperandIndex arg = un.arg;
          un.arg = add_operand(OperandRegister{Register::T0});
          instrs.insert(instrs.begin() + i,
                        add_instruction(InstructionLoad{arg, un.arg}));
          ++i; // skip over just created
        }
      }

      { // Open new because we may have inserted into instrs, which invalidates instr
        auto& instr = instrs[i].from(instructions);
        auto& un = instr.get<UNARY>();
        auto& dest = un.destination.from(operands);

        if (dest.is_a(OperandTag::STACK_ADDR)) {
          OperandIndex dest = un.destination;
          un.destination = add_operand(OperandRegister{Register::T0});
          instrs.insert(
              instrs.begin() + (i + 1),
              add_instruction(InstructionStore{un.destination, dest}));
          ++i; // skip over just created
        }
      }
    } break;
    case ALLOCA: [[fallthrough]];
    case RET: [[fallthrough]];
    case LAST: break;
    }
  }
}

bool AsmParser::suitable_instruction(tac::Tag tag) {
  switch (tag) {
    using enum tac::Tag;
  case RETURN: [[fallthrough]];
  case UNARY: //
    return true;
  case VAR: [[fallthrough]];
  case CONSTANT: [[fallthrough]];
  case PROGRAM: [[fallthrough]];
  case FUNCTION: [[fallthrough]];
  case IDENTIFIER: [[fallthrough]];
  case LAST: //
    return false;
  }
}

asm_ast::InstructionIndex
AsmParser::instruction(const Tac& tac,
                       std::vector<asm_ast::InstructionIndex>& loc) {
  switch (tac.get_tag()) {
    using enum tac::Tag;
  case RETURN: {
    const auto& ret = tac.get<RETURN>();
    const auto& val = ret.value.from(tac_container);
    OperandIndex value_op = forced_operand(val, loc);
    loc.push_back(add_instruction(
        InstructionLoad{value_op, add_operand(OperandRegister{Register::A0})}));
    return loc.emplace_back(add_instruction(InstructionRet{}));
  }
  case UNARY: {
    const auto& unary = tac.get<UNARY>();
    const auto& arg = unary.source.from(tac_container);
    OperandIndex arg_operand = forced_operand(arg, loc);
    UnaryOp op = UnaryOp::LAST;
    switch (unary.op) {
    case token::TokenType::SYM_MINUS: op = UnaryOp::NEGATION; break;
    case token::TokenType::SYM_BANG: op = UnaryOp::BINARY_NOT; break;

    case token::TokenType::UNKNOWN: [[fallthrough]];
    case token::TokenType::POISON: [[fallthrough]];
    case token::TokenType::END: [[fallthrough]];
    case token::TokenType::WHITESPACE: [[fallthrough]];
    case token::TokenType::COMMENT: [[fallthrough]];
    case token::TokenType::IDENTIFIER: [[fallthrough]];
    case token::TokenType::INTEGER: [[fallthrough]];
    case token::TokenType::KW_DEF: [[fallthrough]];
    case token::TokenType::KW_FUN: [[fallthrough]];
    case token::TokenType::KW_RETURN: [[fallthrough]];
    case token::TokenType::SYM_MINUS_MINUS: [[fallthrough]];
    case token::TokenType::SYM_BANG_BANG: [[fallthrough]];
    case token::TokenType::SYM_STRONG_ARROW_RIGHT: [[fallthrough]];
    case token::TokenType::SYM_OPEN_PAREN: [[fallthrough]];
    case token::TokenType::SYM_CLOSE_PAREN: [[fallthrough]];
    case token::TokenType::SYM_OPEN_BRACE: [[fallthrough]];
    case token::TokenType::SYM_CLOSE_BRACE: [[fallthrough]];
    case token::TokenType::SYM_SEMICOLON: [[fallthrough]];
    case token::TokenType::SYM_EQUAL: [[fallthrough]];
    case token::TokenType::SYM_PLUS: [[fallthrough]];
    case token::TokenType::SYM_PLUS_PLUS: [[fallthrough]];
    case token::TokenType::SYM_ASTERISK: [[fallthrough]];
    case token::TokenType::SYM_SLASH: [[fallthrough]];
    case token::TokenType::SYM_PERCENT: [[fallthrough]];
    case token::TokenType::SYM_GREATER_THAN: [[fallthrough]];
    case token::TokenType::SYM_GREATER_THAN_GREATER_THAN: [[fallthrough]];
    case token::TokenType::SYM_GREATER_THAN_EQUAL: [[fallthrough]];
    case token::TokenType::SYM_LESS_THAN: [[fallthrough]];
    case token::TokenType::SYM_LESS_THAN_LESS_THAN: [[fallthrough]];
    case token::TokenType::SYM_LESS_THAN_EQUAL: [[fallthrough]];
    case token::TokenType::SYM_AMPERSAND: [[fallthrough]];
    case token::TokenType::SYM_PIPE: [[fallthrough]];
    case token::TokenType::SYM_CARET: [[fallthrough]];
    case token::TokenType::LAST: unreachable;
    }
    Operand dest = OperandPseudo{std::format("asm.{}", operands.size())};
    OperandIndex dest_idx = add_operand(dest);

    return loc.emplace_back(
        add_instruction(InstructionUnary{op, arg_operand, dest_idx}));
  }
  // TODO Add errors? Do we need them?
  case PROGRAM: [[fallthrough]];
  case FUNCTION: [[fallthrough]];
  case CONSTANT: [[fallthrough]];
  case VAR: [[fallthrough]];
  case IDENTIFIER: [[fallthrough]];
  case LAST: unreachable;
  }
}

OperandIndex AsmParser::get_operand(InstructionIndex idx) {
  const auto& instr = idx.from(instructions);

  switch (instr.get_tag()) {
    using enum InstructionTag;
  case LOAD: return instr.get<LOAD>().destination;
  case STORE: return instr.get<STORE>().destination;
  case UNARY: return instr.get<UNARY>().destination;
  // TODO Feex
  case ALLOCA: [[fallthrough]];
  case RET: [[fallthrough]];
  case LAST: //
    unreachable;
  }
}

bool AsmParser::suitable_operand(tac::Tag tag) {
  switch (tag) {
    using enum tac::Tag;
  case CONSTANT: [[fallthrough]];
  case VAR: //
    return true;
  case PROGRAM: [[fallthrough]];
  case FUNCTION: [[fallthrough]];
  case RETURN: [[fallthrough]];
  case UNARY: [[fallthrough]];
  case IDENTIFIER: [[fallthrough]];
  case LAST: //
    return false;
  }
}

OperandIndex AsmParser::operand(const Tac& tac) {
  switch (tac.get_tag()) {
    using enum tac::Tag;
  case CONSTANT:
    return add_operand(OperandImmediate{tac.get<CONSTANT>().value});
  case VAR: //
    return add_operand(OperandPseudo{tac.get<VAR>().var});
  // TODO ERRORS! !!! !!! !
  case IDENTIFIER: [[fallthrough]];
  case RETURN: [[fallthrough]];
  case UNARY: [[fallthrough]];
  // TODO Add errors instead of crashing :)
  case PROGRAM: [[fallthrough]];
  case FUNCTION: [[fallthrough]];
  case LAST: //
    unreachable;
  }
}

OperandIndex AsmParser::forced_operand(const Tac& tac,
                                       std::vector<InstructionIndex>& loc) {
  if (suitable_operand(tac.get_tag())) {
    return operand(tac);
  } else if (suitable_instruction(tac.get_tag())) {
    auto instr = instruction(tac, loc);
    return get_operand(instr);
  } else {
    // TODO ERROROROROOROOOOORRRRR
    unreachable;
  }
}

InstructionIndex AsmParser::add_instruction(const Instruction& inst) {
  return instructions.push_back(inst);
}

OperandIndex AsmParser::add_operand(const Operand& op) {
  return operands.push_back(op);
}