#pragma once

#include <vector>

#include "common/token.hpp"
#include "util/assert.hpp"

#include "util/self_referential_container.hpp"
#include "util/tagged_union.hpp"

namespace ast {
enum class Tag {
  NONE,
  INTEGER,
  IDENTIFIER,
  RETURN,
  PRE_OP,
  LIST,
  FUNCTION,

  LAST,
  FIRST = NONE
};

static_assert(constrained_enum<Tag>, "ast::Tag is not a constrained union");
} // namespace ast

template <>
struct std::formatter<ast::Tag> {
  constexpr auto parse(std::format_parse_context& ctx) {
    return ctx.begin();
  }

  constexpr auto format(const ast::Tag& id, std::format_context& ctx) const {
    switch (id) {
      using enum ast::Tag;
    case NONE:
      return std::format_to(ctx.out(), "NONE");
    case INTEGER:
      return std::format_to(ctx.out(), "INTEGER");
    case IDENTIFIER:
      return std::format_to(ctx.out(), "IDENTIFIER");
    case RETURN:
      return std::format_to(ctx.out(), "RETURN");
    case PRE_OP:
      return std::format_to(ctx.out(), "PRE_OP");
    case LIST:
      return std::format_to(ctx.out(), "LIST");
    case FUNCTION:
      return std::format_to(ctx.out(), "FUNCTION");
    case LAST:
      return std::format_to(ctx.out(), "LAST");
    }
  }
};

namespace ast {

template <Tag tag>
struct Tagged {
  constexpr static Tag union_tag() {
    return tag;
  }
};

class Ast;

using AstIndex = SRContainerIndex<Ast>;
using AstContainer = SelfReferentialContainer<Ast>;

/** Empty AST node */
struct AstNone : Tagged<Tag::NONE> {
  AstNone() {}
};

/** AST node that contains a token */
struct AstToken {
  AstToken(const token::Token& t) : t{t} {}
  token::Token t;
};

/** AST node that contains a compile time integer */
struct AstInteger : AstToken, Tagged<Tag::INTEGER> {
  using AstToken::AstToken;
};

/** AST node that contains an nn identifier */
struct AstIdentifier : AstToken, Tagged<Tag::IDENTIFIER> {
  using AstToken::AstToken;
};

/** Base struct for AST nodes that contain one other AST node */
struct AstUnary {
  AstUnary(const token::Token& t, AstIndex idx) : t{t}, child{idx} {}
  token::Token t;
  AstIndex child;
};

/** AST node representing a return statement */
struct AstReturn : AstUnary, Tagged<Tag::RETURN> {
  using AstUnary::AstUnary;
};

struct AstPreOp : AstUnary, Tagged<Tag::PRE_OP> {
  using AstUnary::AstUnary;
};

/** Base struct for AST nodes that contain two other AST nodes */
struct AstBinary {
  AstBinary(const token::Token& t, AstIndex lhs, AstIndex rhs)
      : t{t}, lhs{lhs}, rhs{rhs} {}

  token::Token t;

  AstIndex lhs;
  AstIndex rhs;
};

/** Base struct for AST nodes that contain a list of other AST nodes */
struct AstList : Tagged<Tag::LIST> {
  AstList(const std::vector<AstIndex>& asts = {}) : asts{asts} {}

  std::vector<AstIndex> asts;
};

/** AST node that represents a runnable function */
struct AstFunction : Tagged<Tag::FUNCTION> {
  AstFunction(const token::Token& t, AstIndex name, AstIndex body)
      : t{t}, name{name}, body{body} {}

  token::Token t;
  AstIndex name;
  AstIndex body;
};

/** Variant that contains all possible ASTs */
using AstUnion = TaggedUnion<Tag, AstNone, AstInteger, AstIdentifier, AstReturn,
                             AstPreOp, AstList, AstFunction>;

/** Wraps over an AST node variant */
class Ast {
public:
  /** Default constructor is an AstNone */
  Ast();

  template <typename T>
    requires(AstUnion::contains_type<T>())
  Ast(const T& t) : data{t} {}

  /** Gets the display name of this AST node type */
  const char* get_name() const;
  /** Returns the main token for this AST node */
  std::optional<token::Token> main_token(const AstContainer& container) const;
  /** Returns the source location for this AST node */
  source::SourceLocation source_location(const AstContainer& container) const;

  Tag get_tag() const;

  template <has_tag<Tag> T>
  bool is_a() const {
    return data.get_tag() == T::union_tag();
  }

  bool is_a(Tag tag) const;

  template <has_tag<Tag> T>
  void require() const {
    nn_assert(is_a<T>());
  }

  void require(Tag tag) const;

  template <typename T>
  auto& get() {
    return data.get<T>();
  }

  template <Tag tag>
  auto& get() {
    return data.get<tag>();
  }

  template <typename T>
  const auto& get() const {
    return data.get<T>();
  }

  template <Tag tag>
  const auto& get() const {
    return data.get<tag>();
  }

private:
  /** Inner AST struct */
  AstUnion data;
};

} // namespace ast