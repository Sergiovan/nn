#pragma once

#include <format>
#include <string>

#include "common/ast.hpp"
#include "common/error.hpp"
#include "frontend/lexer.hpp"

namespace parser {

/* Converts a token stream into an ast */
class Parser {
public:
  /* Create a parser for a given lexer, with the given error manager. 
     The lexer determines the source being parsed */
  Parser(lexer::Lexer& lexer, error::ErrorManager& error_manager);

  /* Parse all tokens from the given lexer and create an ast from it */
  std::pair<ast::Ast, const ast::AstContainer&> parse();

private:
  /* Recursive descent. Each function corresponds with a grammar
     construct. It consumes tokens expecting to find that grammar 
     construct, and returns an ast for it */

  /* Top level statement */
  ast::AstIndex top_level_statement();
  ast::AstIndex def_statement();

  ast::AstIndex function_definition();

  ast::AstIndex block();

  ast::AstIndex statement();
  ast::AstIndex return_statement();

  /* Disambiguates expressions from assignments, since assignments are not 
     expressions in nn. It parses an expression until it finds an `=`, then it 
     turns into an assignment */
  ast::AstIndex expression_or_assignment();
  ast::AstIndex expression();

  ast::AstIndex identifier();
  ast::AstIndex integer();

  // Other functions
  /* Looks at the current token without consuming it, that is, without advancing 
     the lexer state again */
  token::Token peek();
  /* Gets the current token and advances the lexer state */
  token::Token consume();

  /* Adds an AST to the list to be able to reference it */
  ast::AstIndex add_ast(const ast::Ast& ast);
  /* Gets an AST object from its index */
  ast::Ast& get(ast::AstIndex idx);

  /* Consumes a token, expecting something specific. Will generate an error if 
     the token consumed is not as expected */
  template <token::TokenType T, token::TokenType... Ts>
  token::Token consume_expect() {
    expect<T, Ts...>();
    return consume();
  }

  /* Consumes a token, requiring it to be something specific. Will crash if the 
     token consumed is not as required */
  template <token::TokenType T, token::TokenType... Ts>
  token::Token consume_require() {
    require<T, Ts...>();
    return consume();
  }

  /* Adds an error to the error manager and generates an empty ast node */
  ast::AstIndex error(const std::string& str);

  /* Adds an error from the given format to the error manager and generates
     an empty ast node */
  template <typename... Args>
  ast::AstIndex error(std::format_string<Args...> fmt, Args&&... args) {
    std::string err = std::format(fmt, std::forward<Args>(args)...);
    return error(err);
  }

  /* Adds an unexpected token type error to the error manager and returns an
     empty ast node */
  template <token::TokenType T, token::TokenType... Ts>
  ast::AstIndex
  error_token_expected(token::TokenType tt,
                       const std::optional<token::Token>& t = std::nullopt) {
    std::stringstream ss;
    std::print(ss, "{}", T);
    (std::print(ss, ", {}", Ts), ...);
    error_manager.add_error(error::Error{
        t.value_or(peek()),
        std::format("Expected one of {}, but got {} instead", ss.str(), tt)});
    return add_ast(ast::Ast{}); // TODO Better error
  }

  /* Returns true if the current token is of the given type, false otherwise */
  bool is(token::TokenType t);

  /* Returns true if the current token is of any of the given types, 
     false otherwise */
  template <token::TokenType T, token::TokenType... Ts>
  bool is() {
    return is(T) || (is(Ts) || ...);
  }

  /* Returns true if the current token is of the given type, adds an error and returns 
     false otherwise */
  bool expect(token::TokenType t);

  /* Returns true if the current token is of one of the given types, adds an error and
     returns false otherwise */
  template <token::TokenType T, token::TokenType... Ts>
  bool expect() {
    if (!is<T, Ts...>()) {
      error_token_expected<T, Ts...>(peek().tt);
      return false;
    }
    return true;
  }

  /* Crashes if the current token is not of the given type */
  void require(token::TokenType t);

  /* Crashes if the current token is not of one of the given types */
  template <token::TokenType T, token::TokenType... Ts>
  void require() {
    nn_assert(is(T) || (is(Ts) || ...));
  }

  /* Token stream provider for the source being parsed */
  lexer::Lexer& lexer;
  /* Error manager for this source */
  error::ErrorManager& error_manager;
  /* Current token being parsed */
  std::optional<token::Token> current;
  /* Container for Asts */
  ast::AstContainer container;
};

} // namespace parser