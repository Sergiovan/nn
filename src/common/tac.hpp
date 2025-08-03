#pragma once

#include <vector>

#include "common/token.hpp"
#include "util/self_referential_container.hpp"
#include "util/tagged_union.hpp"

namespace tac {
enum class Tag {
  PROGRAM,
  FUNCTION,
  RETURN,
  UNARY,
  CONSTANT,
  VAR,
  IDENTIFIER,

  LAST,
  FIRST = PROGRAM,
};

}

template <>
struct std::formatter<tac::Tag> {
  constexpr auto parse(std::format_parse_context& ctx) {
    return ctx.begin();
  }

  constexpr auto format(const tac::Tag& id, std::format_context& ctx) const {
    switch (id) {
      using enum tac::Tag;
    case PROGRAM: return std::format_to(ctx.out(), "PROGRAM");
    case FUNCTION: return std::format_to(ctx.out(), "FUNCTION");
    case RETURN: return std::format_to(ctx.out(), "RETURN");
    case UNARY: return std::format_to(ctx.out(), "UNARY");
    case CONSTANT: return std::format_to(ctx.out(), "CONSTANT");
    case VAR: return std::format_to(ctx.out(), "VAR");
    case IDENTIFIER: return std::format_to(ctx.out(), "IDENTIFIER");
    case LAST: return std::format_to(ctx.out(), "LAST");
    }
  }
};

namespace tac {

class Tac;

using TacIndex = SRContainerIndex<Tac>;
using TacContainer = SelfReferentialContainer<Tac>;

struct TacProgram : Tagged<Tag::PROGRAM> {
  TacProgram(TacIndex function) : function{function} {}
  TacIndex function;
};

struct TacFunction : Tagged<Tag::FUNCTION> {
  TacFunction(const std::string& name) : name{name}, instructions{} {}

  std::string name;
  std::vector<TacIndex> instructions;
};

struct TacReturn : Tagged<Tag::RETURN> {
  TacReturn(TacIndex value) : value{value} {}
  TacIndex value;
};

struct TacUnary : Tagged<Tag::UNARY> {
  TacUnary(token::TokenType op, TacIndex source) : op{op}, source{source} {}
  token::TokenType op;
  TacIndex source;
};

struct TacConstant : Tagged<Tag::CONSTANT> {
  TacConstant(u64 value) : value{value} {}
  TacConstant(s64 value) : value{std::bit_cast<u64>(value)} {}
  u64 value;
};

struct TacVar : Tagged<Tag::VAR> {
  TacVar(TacIndex var) : var{var} {}
  TacIndex var;
};

struct TacIdentifier : Tagged<Tag::IDENTIFIER> {
  TacIdentifier(const std::string& iden) : iden{iden} {}
  std::string iden;
};

using TacUnion = TaggedUnion<Tag, TacProgram, TacFunction, TacReturn, TacUnary,
                             TacConstant, TacVar, TacIdentifier>;

class Tac : public TacUnion {
public:
  using TacUnion::TacUnion;
  Tac() = delete;
};

} // namespace tac