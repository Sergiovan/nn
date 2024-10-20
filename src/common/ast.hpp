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

struct AstBase {
  template <typename Self>
  constexpr const char* get_name(this Self&& self) {
    return std::remove_reference_t<Self>::_name;
  }
};

template <typename T>
concept AstLike = std::is_base_of_v<AstBase, T> && requires(T t) {
  T::_name;
  { t.get_name() } -> std::same_as<const char*>;
};

struct AstNone : AstBase {
  constexpr static char _name[] = "AstNone";
};
static_assert(AstLike<AstNone>);

struct AstToken : AstBase {
  constexpr static char _name[] = "AstToken";

  AstToken(token::Token t);

  token::Token t;
};
static_assert(AstLike<AstToken>);

struct AstInteger : AstToken {
  constexpr static char _name[] = "AstInteger";

  using AstToken::AstToken;
};
static_assert(AstLike<AstInteger>);

struct AstIdentifier : AstToken {
  constexpr static char _name[] = "AstIdentifier";

  using AstToken::AstToken;
};
static_assert(AstLike<AstIdentifier>);

struct AstUnary : AstBase {
  constexpr static char _name[] = "AstUnary";

  AstUnary(Ast&& other);

  AstPtr child;
};
static_assert(AstLike<AstUnary>);

struct AstReturn : AstUnary {
  constexpr static char _name[] = "AstReturn";

  using AstUnary::AstUnary;
};
static_assert(AstLike<AstReturn>);

struct AstBinary : AstBase {
  constexpr static char _name[] = "AstBinary";

  AstBinary(Ast&& lhs, Ast&& rhs);

  AstPtr lhs;
  AstPtr rhs;
};
static_assert(AstLike<AstBinary>);

struct AstList : AstBase {
  constexpr static char _name[] = "AstList";

  AstList();
  AstList(std::vector<AstPtr>&& asts);

  std::vector<AstPtr> asts;
};
static_assert(AstLike<AstList>);

struct AstFunction : AstBase {
  constexpr static char _name[] = "AstFunction";

  AstFunction(Ast&& name, Ast&& body);

  AstPtr name;
  AstPtr body;
};
static_assert(AstLike<AstFunction>);

using AstVariant =
    std::variant<AstNone, AstToken, AstInteger, AstIdentifier, AstUnary,
                 AstReturn, AstBinary, AstList, AstFunction>;

template <typename F>
concept VisitReturnsVoid = requires(F&& f) {
  { std::visit(std::forward<F>(f), AstVariant{}) } -> std::same_as<void>;
};

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
  auto&& visit(F&& f) {
    nn_assert(std::holds_alternative<T>(data));

    return std::visit(std::forward<F>(f), data);
  }

  template <typename F>
  auto&& visit(F&& f) {
    return std::visit(std::forward<F>(f), data);
  }

  AstPtr as_ptr() &&;

private:
  AstVariant data;
};

} // namespace ast