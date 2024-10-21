#include "lexer.hpp"

#include <memory>

#include "common/token.hpp"

using lexer::Lexer;
using token::Token;
using token::TokenType;

Lexer::Lexer(const std::string& content)
    : source{std::make_shared<source::Source>(content)},
      content{source->get()} {
  if (!content.empty()) {
    current_char = content[current_pos];
  }
}

Token Lexer::next() {
  using enum LexerState;

  std::string_view code = content;

  if (finished || code.empty()) {
    return {TokenType::END, {}};
  }

  while (true) {
    if (current_pos >= code.length() && substate == LexerSubState::NONE) {
      finished = true;
      return {token::TokenType::END, {}};
    }

    switch (state) {
    case FIND:
      current_token_start = current_pos;
      token_start_line = current_line;
      token_start_col = current_col;
      handle_find(current_char);
      continue;
    case WHITESPACE:
      if (!handle_whitespace(current_char)) {
        reset_state();
        return {TokenType::WHITESPACE, current_loc()};
      }
      break;
    case COMMENT:
      if (!handle_comment(current_char)) {
        reset_state();
        return {TokenType::COMMENT, current_loc()};
      }
      break;
    case IDENTIFIER:
      if (!handle_identifier(current_char)) {
        auto token_loc = current_loc();
        auto keyword = token_loc.get();

        TokenType tt = TokenType::IDENTIFIER;
        if (keyword == "def") {
          tt = TokenType::KW_DEF;
        } else if (keyword == "fun") {
          tt = TokenType::KW_FUN;
        } else if (keyword == "return") {
          tt = TokenType::KW_RETURN;
        }

        reset_state();
        return {tt, token_loc};
      }
      break;
    case SYMBOL:
      if (!handle_symbol(current_char)) {
        auto token_loc = current_loc();
        auto keyword = token_loc.get();

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

        reset_state();
        return {tt, token_loc};
      }
      break;
    case NUMBER:
      [[fallthrough]];
    case INTEGER:
      if (!handle_number(current_char)) {
        reset_state();
        return {TokenType::INTEGER, current_loc()};
      }
      break;
    case ERROR: {
      current_pos++;
      Token res{TokenType::UNKNOWN, current_loc()};
      reset_state();
      advance_char();
      return res;
    }
    case END:
      finished = true;
      return {TokenType::END, {}};
    }

    advance_char();
  }
}

std::vector<Token> Lexer::collect() {
  std::vector<Token> res{};

  while (!finished) {
    res.push_back(next());
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

void Lexer::reset_state() {
  state = LexerState::FIND;
  substate = LexerSubState::NONE;
}

void Lexer::advance_char() {
  if (current_char == '\n') {
    current_line++;
    current_col = 0;
  } else {
    current_col++;
  }

  current_char = content[++current_pos];
}

source::SourceLocation Lexer::current_loc() {
  return {current_token_start, token_start_line, token_start_col,
          current_pos - current_token_start, source};
}