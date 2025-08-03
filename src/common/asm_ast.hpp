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
struct std::formatter<asm_ast::Register> : std::formatter<std::string_view> {
  bool as_source = false;

  constexpr auto parse(std::format_parse_context& ctx) {
    constexpr const char AS_SOURCE[] = "source";

    std::string_view ctx_str{ctx};
    if (ctx_str.starts_with(AS_SOURCE)) {
      as_source = true;
      ctx.advance_to(ctx.begin() + sizeof(AS_SOURCE) - 1);
    }
    return std::formatter<std::string_view>::parse(ctx);
  }

  constexpr auto format(const asm_ast::Register& id,
                        std::format_context& ctx) const {
    const char* res = "LAST";
    switch (id) {
      using enum asm_ast::Register;
      // clang-format off
    case ZERO: res = as_source ? "zero" : "$ZERO"; break;
    case RA  : res = as_source ? "ra"   : "$RA";   break;
    case SP  : res = as_source ? "sp"   : "$SP";   break;
    case X3  : res = as_source ? "x3"   : "$X3";   break;
    case X4  : res = as_source ? "x4"   : "$X4";   break;
    case T0  : res = as_source ? "t0"   : "$T0";   break;
    case X6  : res = as_source ? "x6"   : "$X6";   break;
    case X7  : res = as_source ? "x7"   : "$X7";   break;
    case X8  : res = as_source ? "x8"   : "$X8";   break;
    case X9  : res = as_source ? "x9"   : "$X9";   break;
    case A0  : res = as_source ? "a0"   : "$A0";   break;
    case X12 : res = as_source ? "x12"  : "$X12";  break;
    case X13 : res = as_source ? "x13"  : "$X13";  break;
    case X14 : res = as_source ? "x14"  : "$X14";  break;
    case X15 : res = as_source ? "x15"  : "$X15";  break;
    case X16 : res = as_source ? "x16"  : "$X16";  break;
    case X17 : res = as_source ? "x17"  : "$X17";  break;
    case X18 : res = as_source ? "x18"  : "$X18";  break;
    case X19 : res = as_source ? "x19"  : "$X19";  break;
    case X20 : res = as_source ? "x20"  : "$X20";  break;
    case X21 : res = as_source ? "x21"  : "$X21";  break;
    case X22 : res = as_source ? "x22"  : "$X22";  break;
    case X23 : res = as_source ? "x23"  : "$X23";  break;
    case X24 : res = as_source ? "x24"  : "$X24";  break;
    case X25 : res = as_source ? "x25"  : "$X25";  break;
    case X26 : res = as_source ? "x26"  : "$X26";  break;
    case X27 : res = as_source ? "x27"  : "$X27";  break;
    case X28 : res = as_source ? "x28"  : "$X28";  break;
    case X29 : res = as_source ? "x29"  : "$X29";  break;
    case X30 : res = as_source ? "x30"  : "$X30";  break;
    case X31 : res = as_source ? "x31"  : "$X31";  break;
    case LAST: res = "LAST";
      // clang-format on
    }
    return std::formatter<std::string_view>::format(res, ctx);
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