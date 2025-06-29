#pragma once

#include <memory>
#include <type_traits>
#include <variant>
#include <vector>

#include "common/token.hpp"
#include "util/assert.hpp"

namespace ast {

class Ast;

using AstPtr = std::unique_ptr<Ast>;

/** Base for all AST nodes */
struct AstBase {};

template <typename T>
concept AstLike = std::is_base_of_v<AstBase, T> && requires(const T t) {
  T::_name;
  { t.get_name() } -> std::same_as<const char*>;
  { t.main_token() } -> std::same_as<std::optional<token::Token>>;
  { t.source_location() } -> std::same_as<source::SourceLocation>;
};

/** Empty AST node */
struct AstNone : AstBase {
  constexpr static const char* _name = "AstNone";
  constexpr const char* get_name() const {
    return _name;
  }

  std::optional<token::Token> main_token() const;
  source::SourceLocation source_location() const;
};
static_assert(AstLike<AstNone>);
static_assert(std::is_trivially_copy_constructible_v<AstNone>);
static_assert(std::is_trivially_move_constructible_v<AstNone>);
static_assert(std::is_trivially_copy_assignable_v<AstNone>);
static_assert(std::is_trivially_move_assignable_v<AstNone>);
static_assert(std::is_trivially_destructible_v<AstNone>);

/** AST node that contains a token */
struct AstToken : AstBase {
  constexpr static const char* _name = "AstToken";
  constexpr const char* get_name() const {
    return _name;
  }
  AstToken(token::Token t);

  std::optional<token::Token> main_token() const;
  source::SourceLocation source_location() const;

  token::Token t;
};
static_assert(AstLike<AstToken>);

/** AST node that contains a compile time integer */
struct AstInteger : AstToken {
  constexpr static const char* _name = "AstInteger";
  constexpr const char* get_name() const {
    return _name;
  }
  using AstToken::AstToken;
};
static_assert(AstLike<AstInteger>);

/** AST node that contains an nn identifier */
struct AstIdentifier : AstToken {
  constexpr static const char* _name = "AstIdentifier";
  constexpr const char* get_name() const {
    return _name;
  }
  using AstToken::AstToken;
};
static_assert(AstLike<AstIdentifier>);

/** Base struct for AST nodes that contain one other AST node */
struct AstUnary : AstBase {
  constexpr static const char* _name = "AstUnary";
  constexpr const char* get_name() const {
    return _name;
  }
  AstUnary(token::Token t, Ast&& other);

  std::optional<token::Token> main_token() const;
  source::SourceLocation source_location() const;

  token::Token t;
  AstPtr child;
};
static_assert(AstLike<AstUnary>);

/** AST node representing a return statement */
struct AstReturn : AstUnary {
  constexpr static const char* _name = "AstReturn";
  constexpr const char* get_name() const {
    return _name;
  }
  using AstUnary::AstUnary;
};
static_assert(AstLike<AstReturn>);

/** Base struct for AST nodes that contain two other AST nodes */
struct AstBinary : AstBase {
  constexpr static const char* _name = "AstBinary";
  constexpr const char* get_name() const {
    return _name;
  }
  AstBinary(token::Token t, Ast&& lhs, Ast&& rhs);

  std::optional<token::Token> main_token() const;
  source::SourceLocation source_location() const;

  token::Token t;

  AstPtr lhs;
  AstPtr rhs;
};
static_assert(AstLike<AstBinary>);

/** Base struct for AST nodes that contain a list of other AST nodes */
struct AstList : AstBase {
  constexpr static const char* _name = "AstList";

  AstList();
  AstList(std::vector<AstPtr>&& asts);
  constexpr const char* get_name() const {
    return _name;
  }

  std::optional<token::Token> main_token() const;
  source::SourceLocation source_location() const;
  std::vector<AstPtr> asts;
};
static_assert(AstLike<AstList>);

/** AST node that represents a runnable function */
struct AstFunction : AstBase {
  constexpr static const char* _name = "AstFunction";

  AstFunction(token::Token t, Ast&& name, Ast&& body);

  std::optional<token::Token> main_token() const;
  source::SourceLocation source_location() const;

  constexpr const char* get_name() const {
    return _name;
  }
  token::Token t;
  AstPtr name;
  AstPtr body;
};
static_assert(AstLike<AstFunction>);

/** Variant that contains all possible ASTs */
using AstVariant =
    std::variant<AstNone, AstToken, AstInteger, AstIdentifier, AstUnary,
                 AstReturn, AstBinary, AstList, AstFunction>;

/** Wraps over an AST node variant */
class Ast {
public:
  /** Default constructor is an AstNone */
  Ast();

  Ast(const Ast&) = delete;
  Ast& operator=(const Ast&) = delete;

  Ast(Ast&&) = default;
  Ast& operator=(Ast&&) = default;

  template <AstLike T>
  Ast(T&& other) : data{std::move(other)} {}

  template <AstLike T>
  Ast& operator=(T&& other) {
    data = std::move(other);
    return *this;
  }

  /** Gets the inner AST struct */
  AstBase& get();

  /** Gets the display name of this AST node type */
  const char* get_name();
  /** Returns the main token for this AST node */
  std::optional<token::Token> main_token() const;
  /** Returns the source location for this AST node */
  source::SourceLocation source_location() const;

  /** Gets the inner AST struct with the proper type, if that is
      the current AST struct */
  template <AstLike T>
  T& get() {
    nn_assert(std::holds_alternative<T>(data));

    return std::get<T>(data);
  }

  /** Conditionally gets the inner AST struct if it is of a certain
      type, otherwise nullopt */
  template <AstLike T>
  std::optional<T*> get_if() {
    if (std::holds_alternative<T>(data)) {
      return std::get<T>(data);
    } else {
      return std::nullopt;
    }
  }

  /** Invokes a function on the inner AST struct if it is of the 
      proper type */
  template <AstLike T, std::invocable<T&> F>
  auto visit(F&& f) -> std::invoke_result_t<F, T&> {
    nn_assert(std::holds_alternative<T>(data));

    return std::visit(std::forward<F>(f), data);
  }

  /** Invokes a function on the inner AST struct if it is of the 
      proper type */
  template <AstLike T, std::invocable<T&> F>
  auto visit(F&& f) const -> std::invoke_result_t<F, const T&> {
    nn_assert(std::holds_alternative<T>(data));

    return std::visit(std::forward<F>(f), data);
  }

  /** Invokes a function on the inner AST struct */
  template <typename F>
  auto visit(F&& f) -> std::invoke_result_t<F, AstNone&> {
    return std::visit(std::forward<F>(f), data);
  }

  /** Invokes a function on the inner AST struct */
  template <typename F>
  auto visit(F&& f) const -> std::invoke_result_t<F, const AstNone&> {
    return std::visit(std::forward<F>(f), data);
  }

  /* Converts this into an AstPtr */
  AstPtr as_ptr() &&;

private:
  /** Inner AST struct */
  AstVariant data;
};

} // namespace ast