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

struct AstBase {};

template <typename T>
concept AstLike = std::is_base_of_v<AstBase, T> && requires(T t) {
  T::_name;
  { t.get_name() } -> std::same_as<const char*>;
  { t.main_token() } -> std::same_as<std::optional<token::Token>>;
  { t.source_location() } -> std::same_as<source::SourceLocation>;
};

struct AstNone : AstBase {
  constexpr static const char* _name = "AstNone";
  constexpr const char* get_name() {
    return _name;
  }

  std::optional<token::Token> main_token();
  source::SourceLocation source_location();
};
static_assert(AstLike<AstNone>);
static_assert(std::is_trivially_copy_constructible_v<AstNone>);
static_assert(std::is_trivially_move_constructible_v<AstNone>);
static_assert(std::is_trivially_copy_assignable_v<AstNone>);
static_assert(std::is_trivially_move_assignable_v<AstNone>);
static_assert(std::is_trivially_destructible_v<AstNone>);
struct AstToken : AstBase {
  constexpr static const char* _name = "AstToken";
  constexpr const char* get_name() {
    return _name;
  }
  AstToken(token::Token t);

  std::optional<token::Token> main_token();
  source::SourceLocation source_location();

  token::Token t;
};
static_assert(AstLike<AstToken>);

struct AstInteger : AstToken {
  constexpr static const char* _name = "AstInteger";
  constexpr const char* get_name() {
    return _name;
  }
  using AstToken::AstToken;
};
static_assert(AstLike<AstInteger>);

struct AstIdentifier : AstToken {
  constexpr static const char* _name = "AstIdentifier";
  constexpr const char* get_name() {
    return _name;
  }
  using AstToken::AstToken;
};
static_assert(AstLike<AstIdentifier>);

struct AstUnary : AstBase {
  constexpr static const char* _name = "AstUnary";
  constexpr const char* get_name() {
    return _name;
  }
  AstUnary(token::Token t, Ast&& other);

  std::optional<token::Token> main_token();
  source::SourceLocation source_location();

  token::Token t;
  AstPtr child;
};
static_assert(AstLike<AstUnary>);

struct AstReturn : AstUnary {
  constexpr static const char* _name = "AstReturn";
  constexpr const char* get_name() {
    return _name;
  }
  using AstUnary::AstUnary;
};
static_assert(AstLike<AstReturn>);

struct AstBinary : AstBase {
  constexpr static const char* _name = "AstBinary";
  constexpr const char* get_name() {
    return _name;
  }
  AstBinary(token::Token t, Ast&& lhs, Ast&& rhs);

  std::optional<token::Token> main_token();
  source::SourceLocation source_location();

  token::Token t;

  AstPtr lhs;
  AstPtr rhs;
};
static_assert(AstLike<AstBinary>);

struct AstList : AstBase {
  constexpr static const char* _name = "AstList";

  AstList();
  AstList(std::vector<AstPtr>&& asts);
  constexpr const char* get_name() {
    return _name;
  }

  std::optional<token::Token> main_token();
  source::SourceLocation source_location();
  std::vector<AstPtr> asts;
};
static_assert(AstLike<AstList>);

struct AstFunction : AstBase {
  constexpr static const char* _name = "AstFunction";

  AstFunction(token::Token t, Ast&& name, Ast&& body);

  std::optional<token::Token> main_token();
  source::SourceLocation source_location();

  constexpr const char* get_name() {
    return _name;
  }
  token::Token t;
  AstPtr name;
  AstPtr body;
};
static_assert(AstLike<AstFunction>);

using AstVariant =
    std::variant<AstNone, AstToken, AstInteger, AstIdentifier, AstUnary,
                 AstReturn, AstBinary, AstList, AstFunction>;

class Ast {
public:
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

  AstBase& get();

  const char* get_name();
  std::optional<token::Token> main_token();
  source::SourceLocation source_location();

  template <AstLike T>
  T& get() {
    nn_assert(std::holds_alternative<T>(data));

    return std::get<T>(data);
  }

  template <AstLike T>
  std::optional<T*> get_if() {
    if (std::holds_alternative<T>(data)) {
      return std::get<T>(data);
    } else {
      return std::nullopt;
    }
  }

  template <AstLike T, std::invocable<T&> F>
  auto visit(F&& f) -> std::invoke_result_t<F, T&> {
    nn_assert(std::holds_alternative<T>(data));

    return std::visit(std::forward<F>(f), data);
  }

  template <typename F>
  auto visit(F&& f) -> std::invoke_result_t<F, AstNone&> {
    return std::visit(std::forward<F>(f), data);
  }

  AstPtr as_ptr() &&;

private:
  AstVariant data;
};

} // namespace ast