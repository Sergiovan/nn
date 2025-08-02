#include "frontend/parser.hpp"

#include "common/ast.hpp"
#include "common/token.hpp"
#include "util/assert.hpp"

using namespace parser;

using ast::Ast;
using ast::AstIndex;

using lexer::Lexer;

using token::Token;
using token::TokenType;
using enum TokenType;

Parser::Parser(Lexer& lexer, error::ErrorManager& error_manager)
    : lexer{lexer}, error_manager{error_manager}, current{std::nullopt} {}

ParseResult Parser::parse() {
  AstIndex res = program();

  return {res.from(container), container};
}

AstIndex Parser::integer() {
  if (is<INTEGER>()) {
    return add_ast(ast::AstInteger{consume_require<INTEGER>()});
  } else {
    Token t = consume(); // ?
    return error_token_expected<INTEGER>(t.tt, t);
  }
}

AstIndex Parser::identifier() {
  if (is<IDENTIFIER>()) {
    return add_ast(ast::AstIdentifier{consume_require<IDENTIFIER>()});
  } else {
    Token t = consume(); // ?
    return error_token_expected<IDENTIFIER>(t.tt, t);
  }
}

AstIndex Parser::program() {
  std::vector<AstIndex> asts{};

  while (!is(END)) {
    asts.push_back(top_level_statement());
  }

  return add_ast(ast::AstList{asts});
}

AstIndex Parser::top_level_statement() {
  Token t = peek();

  switch (t.tt) {
  case KW_DEF:
    return def_statement();
  default:
    t = consume(); // ?
    return error_token_expected<KW_DEF>(t.tt, t);
  }
}

AstIndex Parser::def_statement() {
  consume_require<KW_DEF>();

  Token t = peek();

  switch (t.tt) {
  case KW_FUN:
    return function_definition();
  default:
    t = consume(); // ?
    return error_token_expected<KW_FUN>(t.tt, t);
  }
}

AstIndex Parser::statement() {
  switch (peek().tt) {
  case KW_RETURN:
    return return_statement();
  default: {
    return expression_or_assignment_statement();
  }
  }
}

AstIndex Parser::return_statement() {
  Token l_return = consume_require<KW_RETURN>();

  AstIndex expr = expression();
  consume_expect<SYM_SEMICOLON>();

  return add_ast(ast::AstReturn{l_return, std::move(expr)});
}

ast::AstIndex Parser::expression_or_assignment_statement() {
  AstIndex ret = expression_or_assignment();
  consume_expect<SYM_SEMICOLON>();
  return ret;
}

AstIndex Parser::function_definition() {
  Token fun = consume_require<KW_FUN>();

  AstIndex name = identifier();
  consume_expect<SYM_OPEN_PAREN>();
  consume_expect<SYM_CLOSE_PAREN>();

  Token t = peek();

  switch (t.tt) {
  case SYM_STRONG_ARROW_RIGHT: {
    Token arrow = consume_require<SYM_STRONG_ARROW_RIGHT>(); // =>
    AstIndex ret = expression();
    consume_expect<SYM_SEMICOLON>();

    ast::AstList body = ast::AstList{};
    body.asts.push_back(add_ast({ast::AstReturn{arrow, ret}}));

    Ast body_ast{body};
    AstIndex body_idx = add_ast(body_ast);

    return add_ast(ast::AstFunction{
        fun,
        name,
        body_idx,
    });
  }
  case SYM_OPEN_BRACE:
    return add_ast(ast::AstFunction{
        fun,
        name,
        block(),
    });
  default:
    t = consume(); // ?
    return add_ast(ast::AstFunction{
        fun,
        name,
        error_token_expected<SYM_STRONG_ARROW_RIGHT, SYM_OPEN_BRACE>(t.tt, t),
    });
  }
}

AstIndex Parser::block() {
  consume_require<SYM_OPEN_BRACE>();

  ast::AstList body = ast::AstList{};

  while (!is<END, SYM_CLOSE_BRACE>()) {
    body.asts.push_back(statement());
  }

  consume_expect<SYM_CLOSE_BRACE>();

  return add_ast(body);
}

AstIndex Parser::expression_or_assignment() {
  return expression(); // :)
}

AstIndex Parser::expression() {
  Token t = peek();

  if (is_prefix_operator()) {
    return pre_expression();
  } else {
    return expression_atom();
  }
}

ast::AstIndex Parser::expression_atom() {
  Token t = peek();

  switch (t.tt) {
  case SYM_OPEN_PAREN: {
    consume_require<SYM_OPEN_PAREN>(); // (
    AstIndex ret = expression();
    consume_expect<SYM_CLOSE_PAREN>(); // )
    return ret;
  }
  case INTEGER:
    return integer();
  default:
    t = consume(); // ?
    return error_token_expected<INTEGER, SYM_OPEN_PAREN>(t.tt, t);
  }
}

ast::AstIndex Parser::pre_expression() {
  nn_assert(is_prefix_operator());

  Token pre_op = consume(); // Prefix operator
  AstIndex expr = expression();

  return add_ast(ast::AstPreOp{pre_op, expr});
}

AstIndex Parser::error(const std::string& str) {
  error_manager.add_error(error::Error{str});

  return add_ast({});
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

bool Parser::is_prefix_operator(std::optional<token::Token> tok) {
  auto token = tok.or_else([this]() -> std::optional<token::Token> {
                    return peek();
                  })
                   .value();
  switch (token.tt) {
    using enum token::TokenType;
  case SYM_MINUS:
    [[fallthrough]];
  case SYM_BANG:
    return true;
  default:
    return false;
  }
}

AstIndex Parser::add_ast(const ast::Ast& ast) {
  return container.push_back(ast);
}

Ast& Parser::get(AstIndex idx) {
  return idx.from(container);
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