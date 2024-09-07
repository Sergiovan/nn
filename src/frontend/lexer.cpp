#include "lexer.hpp"
#include "common/token.hpp"

#include <limits>

using lexer::Lexer;
using token::Token;
using token::TokenType;

std::vector<Token> Lexer::lex(std::string_view code) {
  using enum LexerState;

  if (code.empty()) {
    return {};
  }

  u32 start = 0;
  u32 end = 0;

  std::vector<Token> res{};

  u32 i = 0;
  c8 c = code[0];

  auto reset = [this]() {
    state = FIND;
    substate = LexerSubState::NONE;
  };

  auto advance = [&i, &c, &code]() {
    c = code[++i];
  };

  auto add_token = [&res, &start, &end, &code](TokenType type) {
    res.emplace_back(type, std::span{code.substr(start, end - start)});
  };

  while (true) {
    if (i >= code.length()) {
      res.emplace_back(token::TokenType::END, std::span<c8>{});
      break;
    }

    switch (state) {
    case FIND:
      start = i;
      handle_find(c);
      continue;
    case WHITESPACE:
      if (!handle_whitespace(c)) {
        end = i;
        add_token(TokenType::WHITESPACE);
        reset();
        continue;
      }
      break;
    case COMMENT:
      if (!handle_comment(c)) {
        end = i;
        add_token(TokenType::COMMENT);
        reset();
        continue;
      }
      break;
    case IDENTIFIER:
      if (!handle_identifier(c)) {
        end = i;
        auto keyword = code.substr(start, end - start);
        TokenType tt = TokenType::IDENTIFIER;
        if (keyword == "def") {
          tt = TokenType::KW_DEF;
        } else if (keyword == "fun") {
          tt = TokenType::KW_FUN;
        } else if (keyword == "return") {
          tt = TokenType::KW_RETURN;
        }
        res.emplace_back(tt, keyword);
        reset();
        continue;
      }
      break;
    case SYMBOL:
      if (!handle_symbol(c)) {
        end = i;
        auto keyword = code.substr(start, end - start);
        TokenType tt = TokenType::POISON;
        if (keyword == "=>") {
          tt = TokenType::SYM_STRONG_ARROW_RIGHT;
        } else if (keyword == "(") {
          tt = TokenType::SYM_OPEN_PAREN;
        } else if (keyword == ")") {
          tt = TokenType::SYM_CLOSE_PAREN;
        } else if (keyword == "{") {
          tt = TokenType::SYM_OPEN_BRACE;
        } else if (keyword == "}") {
          tt = TokenType::SYM_CLOSE_BRACE;
        } else if (keyword == ";") {
          tt = TokenType::SYM_SEMICOLON;
        }
        res.emplace_back(tt, keyword);
        reset();
        continue;
      }
      break;
    case NUMBER:
      [[fallthrough]];
    case INTEGER:
      if (!handle_number(c)) {
        end = i;
        add_token(TokenType::INTEGER);
        reset();
        continue;
      }
      break;
    case ERROR:
      i = std::numeric_limits<decltype(i)>::max();
      continue;
    case END:
      return res;
    }

    advance();
  }

  return res;
}

bool Lexer::had_error() {
  return _had_error;
}

void Lexer::handle_find(c8 c) {
  switch (c) {
    using enum LexerState;
  case ' ':
  case '\t':
  case '\n':
  case '\r':
    state = WHITESPACE;
    break;
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
    state = NUMBER;
    break;
  // case '+':
  case '(':
  case ')':
  case '{':
  case '}':
  case ';':
  case '=':
    // case '/':
    state = SYMBOL;
    break;
  case '\0':
    state = END;
    break;
  default:
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
      state = IDENTIFIER;
    } else {
      state = ERROR;
      _had_error = true;
    }
  }
}

bool Lexer::handle_whitespace(c8 c) {
  switch (c) {
  case ' ':
  case '\t':
  case '\n':
  case '\r':
    return true;
  default:
    return false;
  }
}

bool Lexer::handle_comment(c8 c) {
  switch (c) {
  case '\0':
  case '\n':
    return false;
  default:
    return true;
  }
}

bool Lexer::handle_identifier(c8 c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool Lexer::handle_symbol(c8 c) {
  switch (substate) {
    using enum LexerSubState;
  case NONE:
    switch (c) {
    case '(':
    case ')':
    case '{':
    case '}':
    case ';':
      substate = SYMBOL_DONE;
      return true;
    case '=':
      substate = SYMBOL_EQUAL;
      return true;
    default:
      return false;
    }
  case SYMBOL_EQUAL:
    if (c == '>') {
      substate = SYMBOL_DONE;
      return true;
    }
    return false;
  case SYMBOL_DONE:
    return false;
  }
}

bool Lexer::handle_number(c8 c) {
  return (c >= '0' && c <= '9');
}