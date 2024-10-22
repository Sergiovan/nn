#include "frontend/parser.hpp"

#include <iostream>
#include <print>

#include "common/ast.hpp"
#include "common/token.hpp"
#include "util/assert.hpp"

using namespace parser;

using ast::Ast;
using ast::AstPtr;

using lexer::Lexer;

using token::Token;
using token::TokenType;
using enum TokenType;

Parser::Parser(Lexer& lexer) : lexer{lexer}, current{std::nullopt} {}

Ast Parser::parse() {
  std::vector<AstPtr> asts{};

  while (!is(END)) {
    asts.push_back(top_level_statement().as_ptr());
  }

  return ast::AstList{std::move(asts)};
}

Ast Parser::top_level_statement() {
  Token t = peek();

  switch (t.tt) {
  case KW_DEF:
    return def_statement();
  default:
    consume(); // ?
    return error_token_expected<KW_DEF>(t.tt);
  }
}

Ast Parser::def_statement() {
  consume_require<KW_DEF>();

  Token t = peek();

  switch (t.tt) {
  case KW_FUN:
    return function_definition();
  default:
    consume(); // ?
    return error_token_expected<KW_FUN>(t.tt);
  }
}

Ast Parser::function_definition() {
  Token fun = consume_require<KW_FUN>();

  Ast name = identifier();
  consume_expect<SYM_OPEN_PAREN>();
  consume_expect<SYM_CLOSE_PAREN>();

  Token t = peek();

  switch (t.tt) {
  case SYM_STRONG_ARROW_RIGHT: {
    Token arrow = consume_require<SYM_STRONG_ARROW_RIGHT>(); // =>
    Ast ret = expression();
    consume_expect<SYM_SEMICOLON>();

    ast::AstList body = ast::AstList{};
    body.asts.push_back(Ast{ast::AstReturn{arrow, std::move(ret)}}.as_ptr());

    return ast::AstFunction{
        fun,
        std::move(name),
        std::move(body),
    };
  }
  case SYM_OPEN_BRACE:
    return ast::AstFunction{
        fun,
        std::move(name),
        block(),
    };
  default:
    return ast::AstFunction{
        fun,
        std::move(name),
        error_token_expected<SYM_STRONG_ARROW_RIGHT, SYM_OPEN_BRACE>(t.tt),
    };
  }
}

Ast Parser::block() {
  consume_require<SYM_OPEN_BRACE>();

  ast::AstList body = ast::AstList{};

  while (!is<END, SYM_CLOSE_BRACE>()) {
    body.asts.push_back(statement().as_ptr());
  }

  consume_expect<SYM_CLOSE_BRACE>();

  return body;
}

Ast Parser::statement() {
  switch (peek().tt) {
  case KW_RETURN:
    return return_statement();
  default: {
    Ast ret = expression_or_assignment();
    consume_expect<SYM_SEMICOLON>();
    return ret;
  }
  }
}

Ast Parser::return_statement() {
  Token l_return = consume_require<KW_RETURN>();

  Ast expr = expression();
  consume_expect<SYM_SEMICOLON>();

  return ast::AstReturn{l_return, std::move(expr)};
}

Ast Parser::expression_or_assignment() {
  return expression(); // :)
}

Ast Parser::expression() {
  Token t = peek();

  switch (t.tt) {
  case INTEGER:
    return integer(); // TODO Operators and all that
  default:
    consume(); // ?
    return error_token_expected<INTEGER>(t.tt);
  }
}

Ast Parser::identifier() {
  Token t = consume_require<IDENTIFIER>();
  return ast::AstIdentifier{t};
}

Ast Parser::integer() {
  Token t = consume_require<INTEGER>();
  return ast::AstInteger{t};
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

  return *(current = consume());
}

Token Parser::consume() {
  if (current.has_value()) {
    Token t = *current;
    current = std::nullopt;
    return t;
  }

  Token t = lexer.next();
  do {
    switch (t.tt) {
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

bool Parser::is(TokenType tt) {
  return peek().tt == tt;
}

bool Parser::expect(TokenType tt) {
  if (!is(tt)) {
    error(std::format("Expected {} but got {} instead", tt, peek().tt));
    return false;
  }
  return true;
}

void Parser::require(TokenType tt) {
  nn_assert(is(tt));
}