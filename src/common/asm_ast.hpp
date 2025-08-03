#pragma once

#include "util/self_referential_container.hpp"
#include "util/tagged_union.hpp"
#include <string>

namespace asm_ast {
enum class InstructionTag {
  LOAD,
  STORE,
  UNARY,
  ALLOCA,
  RET,

  LAST,
  FIRST = LOAD
};

enum class OperandTag {
  IMMEDIATE,
  REGISTER,
  PSEUDO,
  STACK_ADDR,

  LAST,
  FIRST = IMMEDIATE,
};

enum class Register {
  // clang-format off
  X0, X1, X2, X3, X4,
  X5, X6, X7, X8, X9,
  X10, X11, X12, X13, X14,
  X15, X16, X17, X18, X19,
  X20, X21, X22, X23, X24,
  X25, X26, X27, X28, X29,
  X30, X31,
  // clang-format on

  ZERO = X0,
  RA = X1,
  SP = X2,
  T0 = X5,
  A0 = X10,

  LAST,
  FIRST = X0,
};

enum class UnaryOp {
  BINARY_NOT,
  NEGATION,

  LAST,
  FIRST = BINARY_NOT,
};

} // namespace asm_ast

template <>
struct std::formatter<asm_ast::OperandTag> {
  constexpr auto parse(std::format_parse_context& ctx) {
    return ctx.begin();
  }

  constexpr auto format(const asm_ast::OperandTag& id,
                        std::format_context& ctx) const {
    switch (id) {
      using enum asm_ast::OperandTag;
    case IMMEDIATE: return std::format_to(ctx.out(), "IMMEDIATE");
    case REGISTER: return std::format_to(ctx.out(), "REGISTER");
    case PSEUDO: return std::format_to(ctx.out(), "PSEUDO");
    case STACK_ADDR: return std::format_to(ctx.out(), "STACK_ADDR");
    case LAST: return std::format_to(ctx.out(), "LAST");
    }
  }
};

template <>
struct std::formatter<asm_ast::InstructionTag> {
  constexpr auto parse(std::format_parse_context& ctx) {
    return ctx.begin();
  }

  constexpr auto format(const asm_ast::InstructionTag& id,
                        std::format_context& ctx) const {
    switch (id) {
      using enum asm_ast::InstructionTag;
    case LOAD: return std::format_to(ctx.out(), "LOAD");
    case STORE: return std::format_to(ctx.out(), "STORE");
    case UNARY: return std::format_to(ctx.out(), "UNARY");
    case ALLOCA: return std::format_to(ctx.out(), "ALLOCA");
    case RET: return std::format_to(ctx.out(), "RET");
    case LAST: return std::format_to(ctx.out(), "LAST");
    }
  }
};

template <>
struct std::formatter<asm_ast::UnaryOp> {
  constexpr auto parse(std::format_parse_context& ctx) {
    return ctx.begin();
  }

  constexpr auto format(const asm_ast::UnaryOp& id,
                        std::format_context& ctx) const {
    switch (id) {
      using enum asm_ast::UnaryOp;
    case BINARY_NOT: return std::format_to(ctx.out(), "BINARY_NOT");
    case NEGATION: return std::format_to(ctx.out(), "NEGATION");
    case LAST: return std::format_to(ctx.out(), "LAST");
    }
  }
};

template <>
struct std::formatter<asm_ast::Register> {
  constexpr auto parse(std::format_parse_context& ctx) {
    return ctx.begin();
  }

  constexpr auto format(const asm_ast::Register& id,
                        std::format_context& ctx) const {
    switch (id) {
      using enum asm_ast::Register;
    case ZERO: return std::format_to(ctx.out(), "$ZERO");
    case RA: return std::format_to(ctx.out(), "$RA");
    case SP: return std::format_to(ctx.out(), "$SP");
    case X3: return std::format_to(ctx.out(), "$X3");
    case X4: return std::format_to(ctx.out(), "$X4");
    case T0: return std::format_to(ctx.out(), "$T0");
    case X6: return std::format_to(ctx.out(), "$X6");
    case X7: return std::format_to(ctx.out(), "$X7");
    case X8: return std::format_to(ctx.out(), "$X8");
    case X9: return std::format_to(ctx.out(), "$X9");
    case A0: return std::format_to(ctx.out(), "$A0");
    case X12: return std::format_to(ctx.out(), "$X12");
    case X13: return std::format_to(ctx.out(), "$X13");
    case X14: return std::format_to(ctx.out(), "$X14");
    case X15: return std::format_to(ctx.out(), "$X15");
    case X16: return std::format_to(ctx.out(), "$X16");
    case X17: return std::format_to(ctx.out(), "$X17");
    case X18: return std::format_to(ctx.out(), "$X18");
    case X19: return std::format_to(ctx.out(), "$X19");
    case X20: return std::format_to(ctx.out(), "$X20");
    case X21: return std::format_to(ctx.out(), "$X21");
    case X22: return std::format_to(ctx.out(), "$X22");
    case X23: return std::format_to(ctx.out(), "$X23");
    case X24: return std::format_to(ctx.out(), "$X24");
    case X25: return std::format_to(ctx.out(), "$X25");
    case X26: return std::format_to(ctx.out(), "$X26");
    case X27: return std::format_to(ctx.out(), "$X27");
    case X28: return std::format_to(ctx.out(), "$X28");
    case X29: return std::format_to(ctx.out(), "$X29");
    case X30: return std::format_to(ctx.out(), "$X30");
    case X31: return std::format_to(ctx.out(), "$X31");
    case LAST: return std::format_to(ctx.out(), "LAST");
    }
  }
};

namespace asm_ast {
class Instruction;

using InstructionIndex = SRContainerIndex<Instruction>;
using InstructionContainer = SelfReferentialContainer<Instruction>;

class Operand;

using OperandIndex = SRContainerIndex<Operand>;
using OperandContainer = SelfReferentialContainer<Operand>;

struct Function {
  std::string name;
  std::vector<InstructionIndex> instructions;
};

struct Program {
  Function fn;
};

struct OperandImmediate : Tagged<OperandTag::IMMEDIATE> {
  OperandImmediate(u64 value) : value{value} {}
  OperandImmediate(s64 value) : value{std::bit_cast<u64>(value)} {}

  u64 value;
};

struct OperandRegister : Tagged<OperandTag::REGISTER> {
  OperandRegister(Register reg) : reg{reg} {}

  Register reg;
};

struct OperandPseudo : Tagged<OperandTag::PSEUDO> {
  template <typename T>
  OperandPseudo(SRContainerIndex<T> t) : pseudo_value{std::format("{}", t)} {}
  OperandPseudo(const std::string& str) : pseudo_value{str} {}

  std::string pseudo_value;
};

struct OperandStackAddress : Tagged<OperandTag::STACK_ADDR> {
  OperandStackAddress(s64 offset) : offset{offset} {}
  s64 offset;
};

using OperandUnion = TaggedUnion<OperandTag, OperandImmediate, OperandRegister,
                                 OperandPseudo, OperandStackAddress>;

class Operand : public OperandUnion {
  using OperandUnion::OperandUnion;
  Operand() = delete;
};

struct InstructionLoad : Tagged<InstructionTag::LOAD> {
  InstructionLoad(OperandIndex source, OperandIndex destination)
      : source{source}, destination{destination} {}

  OperandIndex source;
  OperandIndex destination;
};

struct InstructionStore : Tagged<InstructionTag::STORE> {
  InstructionStore(OperandIndex source, OperandIndex destination)
      : source{source}, destination{destination} {}

  OperandIndex source;
  OperandIndex destination;
};

struct InstructionUnary : Tagged<InstructionTag::UNARY> {
  InstructionUnary(UnaryOp op, OperandIndex arg, OperandIndex destination)
      : op{op}, arg{arg}, destination{destination} {}

  UnaryOp op;
  OperandIndex arg;
  OperandIndex destination;
};

struct InstructionAllocateStack : Tagged<InstructionTag::ALLOCA> {
  InstructionAllocateStack(s64 bytes) : bytes{bytes} {}
  // TODO Check that no stack is too larg...
  InstructionAllocateStack(u64 bytes) : bytes{(signed)bytes} {}

  s64 bytes;
};

struct InstructionRet : Tagged<InstructionTag::RET> {};

using InstructionUnion =
    TaggedUnion<InstructionTag, InstructionLoad, InstructionStore,
                InstructionUnary, InstructionAllocateStack, InstructionRet>;

class Instruction : public InstructionUnion {
  using InstructionUnion::InstructionUnion;
  Instruction() = delete;
};

} // namespace asm_ast