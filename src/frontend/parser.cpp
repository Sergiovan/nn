#include "frontend/parser.hpp"

#include <iostream>
#include <print>

#include "common/ast.hpp"
#include "common/token.hpp"
#include "util/assert.hpp"

using namespace parser;

using ast::Ast;
using lexer::Lexer;
using token::Token;

Parser::Parser(Lexer& lexer) : lexer{lexer}, current{std::nullopt} {}

Ast Parser::parse() {
  std::vector<ast::AstPtr> asts{};

  peek();

  while (!is(token::TokenType::END)) {
    asts.push_back(top_level_statement().as_ptr());
    peek();
  }

  return ast::AstList{std::move(asts)};
}

Ast Parser::top_level_statement() {
  Token t = peek();

  switch (t.tt) {
    using enum token::TokenType;
  case KW_DEF:
    return def_statement();
  default:
    return error("Expected {} but got {} instead", KW_DEF, t.tt);
  }
}

Ast Parser::def_statement() {
  peek();
  assert_is(token::TokenType::KW_DEF);

  get(); // def
  Token t = peek();

  switch (t.tt) {
    using enum token::TokenType;
  case token::TokenType::KW_FUN:
    return function_definition();
  default:
    return error("Expected {} but got {} instead", KW_FUN, t.tt);
  }
}

Ast Parser::function_definition() {
  peek();
  assert_is(token::TokenType::KW_FUN);

  get(); // fun

  Ast name = identifier();
  Token t = peek();
  if (!is(token::TokenType::SYM_OPEN_PAREN)) {
    error("Expected {} but got {} instead", token::TokenType::SYM_OPEN_PAREN,
          t.tt);
  }
  get(); // (
  t = peek();
  if (!is(token::TokenType::SYM_CLOSE_PAREN)) {
    error("Expected {} but got {} instead", token::TokenType::SYM_CLOSE_PAREN,
          t.tt);
  }
  get(); // )
  t = peek();

  switch (t.tt) {
    using enum token::TokenType;
  case SYM_STRONG_ARROW_RIGHT: {
    get(); // =>
    Ast ret = expression();
    t = peek();
    if (!is(SYM_SEMICOLON)) {
      error("Expected {} but got {} instead", SYM_SEMICOLON, t.tt);
    }
    get(); // ;

    ast::AstList body = ast::AstList{};
    body.asts.push_back(Ast{ast::AstReturn{std::move(ret)}}.as_ptr());

    return ast::AstFunction{
        std::move(name),
        std::move(body),
    };
  }
  case SYM_OPEN_BRACE:
    return ast::AstFunction{
        std::move(name),
        block(),
    };
  default:
    return ast::AstFunction{
        std::move(name),
        error("Expected one of {} or {}, but got {} instead",
              SYM_STRONG_ARROW_RIGHT, SYM_OPEN_BRACE, t.tt),
    };
  }
}

Ast Parser::block() {
  peek();
  assert_is(token::TokenType::SYM_OPEN_BRACE);

  ast::AstList body = ast::AstList{};
  get(); // {

  Token t = peek();
  while (!is<token::TokenType::END, token::TokenType::SYM_CLOSE_BRACE>()) {
    body.asts.push_back(statement().as_ptr());
    t = peek();
  }

  if (!is(token::TokenType::SYM_CLOSE_BRACE)) {
    error("Expected {} but got {} instead", token::TokenType::SYM_CLOSE_BRACE,
          t.tt);
  }

  get(); // }

  return body;
}

Ast Parser::statement() {
  Token t = peek();

  switch (t.tt) {
    using enum token::TokenType;
  case KW_RETURN:
    return return_statement();
  default: {
    Ast ret = expression_or_assignment();
    if (!is(SYM_SEMICOLON)) {
      error("Expected {} but got {} instead", token::TokenType::SYM_SEMICOLON,
            t.tt);
    }
    get(); // ;
    return ret;
  }
  }
}

Ast Parser::return_statement() {
  peek();
  assert_is(token::TokenType::KW_RETURN);

  get(); // return
  Ast expr = expression();

  Token t = peek();
  if (!is(token::TokenType::SYM_SEMICOLON)) {
    error("Expected {} but got {} instead", token::TokenType::SYM_SEMICOLON,
          t.tt);
  }
  get(); // ;

  return ast::AstReturn{std::move(expr)};
}

Ast Parser::expression_or_assignment() {
  return expression(); // :)
}

Ast Parser::expression() {
  Token t = peek();

  switch (t.tt) {
    using enum token::TokenType;
  case INTEGER: {
    return integer(); // TODO Operators and all that
  }
  default:
    return error("Cannot make an expression out of {}", t.tt);
  }
}

Ast Parser::identifier() {
  Token t = peek();
  assert_is(token::TokenType::IDENTIFIER);

  Ast identifier = ast::AstIdentifier{t};
  get(); // Identifier
  return identifier;
}

Ast Parser::integer() {
  Token t = peek();
  assert_is(token::TokenType::INTEGER);

  Ast identifier = ast::AstIdentifier{t};
  get(); // Integer
  return identifier;
}

Ast Parser::error(const std::string& str) {
  // TODO Save errors
  std::cout << str << "\n";

  return Ast{};
}

Token Parser::peek() {
  if (current.has_value()) {
    return *current;
  }

  return *(current = get());
}

Token Parser::get() {
  if (current.has_value()) {
    Token t = *current;
    current = std::nullopt;
    return t;
  }

  Token t = lexer.next();
  do {
    switch (t.tt) {
      using enum token::TokenType;
    case WHITESPACE:
      [[fallthrough]];
    case COMMENT:
      break;
    default:
      return t;
    }
    t = lexer.next();
  } while (true);
}

void Parser::assert_is(token::TokenType t) {
  nn_assert(current.has_value());
  nn_assert(current->tt == t);
}

bool Parser::is(token::TokenType t) {
  return current.has_value() && current->tt == t;
}