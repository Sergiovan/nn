#include "asm_parser.hpp"
#include "common/ast.hpp"
#include "util/assert.hpp"

using namespace asm_parser;
using namespace asm_ast;

void AsmBlock::add(std::unique_ptr<asm_ast::AstInstruction> instruction) {
  instructions->push_back(std::move(instruction));
}

AsmParser::AsmParser(const ast::Ast& ast_in,
                     const ast::AstContainer& ast_container)
    : ast_in{ast_in}, ast_container{ast_container} {}

const AstProgram& AsmParser::get() {
  head = program(ast_in);
  return *head;
}

AstProgram AsmParser::program(const ast::Ast& ast) {
  auto& list = ast.get<ast::AstList>();

  for (const auto& ast : ast_container.iterate_content(list.asts)) {
    ast.require<ast::AstFunction>();
    return AstProgram{parse_function_definition(ast)};
  }

  // TODO Not actually unreachable...
  unreachable;
}

AstFunctionDef AsmParser::parse_function_definition(const ast::Ast& ast) {
  AstFunctionDef def{};

  auto& fn = ast.get<ast::AstFunction>();

  def.name = fn.name.from(ast_container).source_location(ast_container).get();
  auto prev = add_block(def.instructions);
  parse_list(fn.body.from(ast_container));
  add_block(prev);

  return def;
}

void AsmParser::parse_list(const ast::Ast& ast) {
  auto& list = ast.get<ast::AstList>();

  for (const auto& ast : ast_container.iterate_content(list.asts)) {
    switch (ast.get_tag()) {
      using enum ast::Tag;
    case RETURN:
      parse_return(ast.get<RETURN>());
      break;
    default:
      unreachable;
    }
  }
}

void AsmParser::parse_return(const ast::Ast& ast) {
  auto& ret = ast.get<ast::AstReturn>();
  auto& child = ret.child.from(ast_container);

  if (child.is_a<ast::AstInteger>()) {
    add_to_block(
        gen_li(gen_integer(child), std::make_unique<AstOperandRegister>()));
  }

  add_to_block(std::make_unique<AstInstructionRet>());
}

std::unique_ptr<asm_ast::AstInstructionMov>
AsmParser::gen_mov(std::unique_ptr<asm_ast::AstOperandRegister> src,
                   std::unique_ptr<asm_ast::AstOperandRegister> dst) {
  return std::make_unique<AstInstructionMov>(std::move(src), std::move(dst));
}

std::unique_ptr<asm_ast::AstInstructionLi>
AsmParser::gen_li(std::unique_ptr<asm_ast::AstOperandImmediate> src,
                  std::unique_ptr<asm_ast::AstOperandRegister> dst) {
  return std::make_unique<AstInstructionLi>(std::move(src), std::move(dst));
}

std::unique_ptr<AstOperandImmediate>
AsmParser::gen_integer(const ast::Ast& ast) {
  return std::make_unique<AstOperandImmediate>(stoll(
      ast.source_location(ast_container)
          .get())); // TODO Might not work if number too large, no limits right now...
}

std::optional<AsmBlock> AsmParser::add_block(
    std::vector<std::unique_ptr<AstInstruction>>& instructions) {
  auto tmp = current_block;
  current_block = {AsmBlock{&instructions}};
  return tmp;
}

std::optional<AsmBlock> AsmParser::add_block(std::optional<AsmBlock>& block) {
  auto tmp = current_block;
  current_block = block;
  return tmp;
}

void AsmParser::add_to_block(std::unique_ptr<AstInstruction> inst) {
  nn_assert(current_block.has_value());

  current_block->add(std::move(inst));
}