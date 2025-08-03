#include "output_gnu_as.hpp"

#include "common/asm_ast.hpp"
#include "util/assert.hpp"

#include <print>
#include <sstream>

using namespace asm_ast;

static std::string handle_immediate_operand(const OperandImmediate& imm) {
  return std::format("0x{:X}", imm.value);
}

static std::string handle_register_operand(const OperandRegister& reg) {
  return std::format("{:source}", reg.reg);
}

static std::string handle_stack_addr_operand(const OperandStackAddress& sa) {
  if (sa.offset) {
    if (sa.offset < 0) {
      return std::format("-0x{:X}({:source})", -sa.offset, Register::SP);
    } else {
      return std::format("0x{:X}({:source})", sa.offset, Register::SP);
    }
  } else {
    return std::format("({:source})", Register::SP);
  }
}

static std::string handle_op(const Operand& op) {
  switch (op.get_tag()) {
    using enum OperandTag;
  case IMMEDIATE: return handle_immediate_operand(op.get<IMMEDIATE>());
  case REGISTER: return handle_register_operand(op.get<REGISTER>());
  case STACK_ADDR: return handle_stack_addr_operand(op.get<STACK_ADDR>());
  case PSEUDO: [[fallthrough]];
  case LAST: unreachable;
  }
}

static void handle_load_instr(std::ostream& os, const InstructionLoad& ld,
                              const OperandContainer& ops) {
  auto& src = ld.source.from(ops);
  auto& dst = ld.destination.from(ops);
  if (src.is_a(OperandTag::IMMEDIATE)) {
    std::println(os, "  li {}, {}", handle_op(dst), handle_op(src));
  } else {
    std::println(os, "  ld {}, {}", handle_op(dst), handle_op(src));
  }
}

static void handle_store_instr(std::ostream& os, const InstructionStore& st,
                               const OperandContainer& ops) {
  auto& src = st.source.from(ops);
  auto& dst = st.destination.from(ops);
  std::println(os, "  sd {}, {}", handle_op(src), handle_op(dst));
}

static void handle_unary_instr(std::ostream& os, const InstructionUnary& un,
                               const OperandContainer& ops) {
  auto& arg = un.arg.from(ops);
  auto& dst = un.destination.from(ops);

  const char* instr = "unknown_op";
  switch (un.op) {
  case UnaryOp::BINARY_NOT: instr = "not"; break;
  case UnaryOp::NEGATION: instr = "neg"; break;
  case UnaryOp::LAST: unreachable;
  }

  std::println(os, "  {} {}, {}", instr, handle_op(dst), handle_op(arg));
}

static void handle_alloca_instr(std::ostream& os,
                                const InstructionAllocateStack& stck,
                                const OperandContainer& ops) {
  if (stck.bytes <= 0) {
    std::println(os, "  addi {:source}, {:source}, 0x{:X}", Register::SP,
                 Register::SP,
                 -stck.bytes); // Allocating grows down, so we negate
  } else {
    std::println(os, "  addi {:source}, {:source}, -0x{:X}", Register::SP,
                 Register::SP, stck.bytes);
  }
}

static void handle_ret_instr(std::ostream& os, const InstructionRet& ret,
                             const OperandContainer& ops) {
  std::println(os, "  ld ra, -0x8(sp)");
  std::println(os, "  ret");
}

std::string to_gnu_as(asm_parser::ParseResult result) {
  const auto& [asm_, instr_container, op_container] = result;

  std::stringstream out;

  std::println(out, ".globl _start");
  std::println(out, ".globl {}", asm_.fn.name);
  std::println(out);
  std::println(out, "_start:");
  std::println(out, "  li sp, 0x80100000"); // Top of the stack
  std::println(out, "  call main");
  std::println(out, "  csrrwi zero, mscratch, 1"); // For testing purposes
  std::println(out, "  ebreak\n\n# Compiled program");
  std::println(out, "{}:", asm_.fn.name);
  std::println(out, "  sd ra, -0x8(sp)");

  for (const auto& instr :
       instr_container.iterate_content(asm_.fn.instructions)) {
    switch (instr.get_tag()) {
      using enum InstructionTag;
    case LOAD: handle_load_instr(out, instr.get<LOAD>(), op_container); break;
    case STORE:
      handle_store_instr(out, instr.get<STORE>(), op_container);
      break;
    case UNARY:
      handle_unary_instr(out, instr.get<UNARY>(), op_container);
      break;
    case ALLOCA:
      handle_alloca_instr(out, instr.get<ALLOCA>(), op_container);
      break;
    case RET: handle_ret_instr(out, instr.get<RET>(), op_container); break;
    case LAST: unreachable;
    }
  }

  std::println(out, "\n.section .note.GNU-stack,\"\",@progbits");

  return out.str();
}