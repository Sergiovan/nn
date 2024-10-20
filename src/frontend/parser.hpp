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
  token::Token get();

  ast::Ast error(const std::string& str);

  template <typename... Args>
  ast::Ast error(std::format_string<Args...> fmt, Args&&... args) {
    std::string err = std::format(fmt, std::forward<Args>(args)...);
    return error(err);
  }

  template <token::TokenType... Ts>
  void assert_is() {
    (assert_is(Ts), ...);
  }

  template <token::TokenType... Ts>
  bool is() {
    return (is(Ts) || ...);
  }

  void assert_is(token::TokenType t);
  bool is(token::TokenType t);

  lexer::Lexer& lexer;
  std::optional<token::Token> current;
};

} // namespace parser