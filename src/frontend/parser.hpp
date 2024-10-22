#pragma once

#include <format>
#include <string>

#include "common/ast.hpp"
#include "frontend/lexer.hpp"

namespace parser {

class Parser {
public:
  Parser(lexer::Lexer& lexer);

  ast::Ast parse();

private:
  // Recursive descent
  ast::Ast top_level_statement();
  ast::Ast def_statement();

  ast::Ast function_definition();

  ast::Ast block();

  ast::Ast statement();
  ast::Ast return_statement();

  ast::Ast expression_or_assignment();
  ast::Ast expression();

  ast::Ast identifier();
  ast::Ast integer();

  // Other functions
  token::Token peek();
  token::Token consume();

  template <token::TokenType T, token::TokenType... Ts>
  token::Token consume_expect() {
    expect<T, Ts...>();
    return consume();
  }

  template <token::TokenType T, token::TokenType... Ts>
  token::Token consume_require() {
    require<T, Ts...>();
    return consume();
  }

  ast::Ast error(const std::string& str);

  template <typename... Args>
  ast::Ast error(std::format_string<Args...> fmt, Args&&... args) {
    std::string err = std::format(fmt, std::forward<Args>(args)...);
    return error(err);
  }

  template <token::TokenType T, token::TokenType... Ts>
  ast::Ast error_token_expected(token::TokenType tt) {
    std::stringstream ss;
    std::print(ss, "{}", T);
    (std::print(ss, ", {}", Ts), ...);
    return error("Expected one of {}, but got {} instead", ss.str(),
                 current->tt);
  }

  bool is(token::TokenType t);

  template <token::TokenType T, token::TokenType... Ts>
  bool is() {
    return is(T) || (is(Ts) || ...);
  }

  bool expect(token::TokenType t);

  template <token::TokenType T, token::TokenType... Ts>
  bool expect() {
    if (!is<T, Ts...>()) {
      error_token_expected<T, Ts...>(peek().tt);
      return false;
    }
    return true;
  }

  void require(token::TokenType t);

  template <token::TokenType T, token::TokenType... Ts>
  void require() {
    nn_assert(is(T) || (is(Ts) || ...));
  }

  lexer::Lexer& lexer;
  std::optional<token::Token> current;
};

} // namespace parser