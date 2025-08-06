#include "lexer.hpp"

#include <memory>

#include "common/error.hpp"
#include "common/token.hpp"

using error::Error;
using error::ErrorManager;
using lexer::Lexer;
using token::Token;
using token::TokenType;

Lexer::Lexer(std::shared_ptr<source::Source> source,
             ErrorManager& error_manager)
    : source{source}, content{source->get()}, error_manager{error_manager} {
  if (!content.empty()) {
    current_char = content[current_pos];
  }
}

Token Lexer::next() {
  using enum LexerState;

  std::string_view code = content;

  // Generate END tokens infinitely after we're done
  if (finished || code.empty()) {
    return {TokenType::END, {.source = source}};
  }

  while (true) {
    if (current_pos >= code.length() && token_substate == TokenType::UNKNOWN) {
      finished = true;
      return {token::TokenType::END, {.source = source}};
    }

    switch (state) {
    case FIND:
      /* Start a token */
      current_token_start = current_pos;
      token_start_line = current_line;
      token_start_col = current_col;
      handle_find(current_char);
      continue;
    case WHITESPACE:
      if (handle_whitespace(current_char)) {
        reset_state();
        return {TokenType::WHITESPACE, current_loc()};
      }
      break;
    case COMMENT:
      if (handle_comment(current_char)) {
        reset_state();
        return {TokenType::COMMENT, current_loc()};
      }
      break;
    case IDENTIFIER:
      if (handle_identifier(current_char)) {
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
      if (handle_symbol(current_char)) {
        TokenType tt = token_substate;
        auto token_loc = current_loc();

        if (tt == TokenType::UNKNOWN || tt == TokenType::POISON) {
          _had_error = true;
          error_manager.add_error(
              Error{{TokenType::UNKNOWN, token_loc}, "Unknown symbol found"});
        }

        reset_state();
        return {tt, token_loc};
      }
      break;
    case NUMBER: [[fallthrough]];
    case INTEGER:
      if (handle_number(current_char)) {
        reset_state();
        return {TokenType::INTEGER, current_loc()};
      }
      break;
    case ERROR: {
      _had_error = true;
      advance_char();
      Token res{TokenType::UNKNOWN, current_loc()};
      error_manager.add_error(Error{res, "Unknown token found"});
      reset_state();
      return res;
    }
    case END: finished = true; return {TokenType::END, {.source = source}};
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
  case '\r': //
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
  case '9': //
    state = NUMBER;
    break;
  case '(':
  case ')':
  case '{':
  case '}':
  case ';':
  case '=':
  case '+':
  case '-':
  case '*':
  case '/':
  case '%':
  case '!':
  case '>':
  case '<':
  case '&':
  case '|':
  case '^': //
    state = SYMBOL;
    break;
  case '\0': //
    state = END;
    break;
  default:
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
      state = IDENTIFIER;
    } else {
      state = ERROR;
    }
  }
}

bool Lexer::handle_whitespace(c8 c) {
  switch (c) {
  case ' ':
  case '\t':
  case '\n':
  case '\r': return false;
  default: return true;
  }
}

bool Lexer::handle_comment(c8 c) {
  switch (c) {
  case '\0':
  case '\n': return true;
  default: return false;
  }
}

bool Lexer::handle_identifier(c8 c) {
  return not((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_');
}

bool Lexer::handle_symbol(c8 c) {
  auto set_substate = [this](TokenType tt) {
    token_substate = tt;
    return false;
  };

  switch (token_substate) {
    using enum TokenType;
  case UNKNOWN:
    switch (c) {
    case '(': return set_substate(SYM_OPEN_PAREN);
    case ')': return set_substate(SYM_CLOSE_PAREN);
    case '{': return set_substate(SYM_OPEN_BRACE);
    case '}': return set_substate(SYM_CLOSE_BRACE);
    case ';': return set_substate(SYM_SEMICOLON);
    case '=': return set_substate(SYM_EQUAL);
    case '+': return set_substate(SYM_PLUS);
    case '-': return set_substate(SYM_MINUS);
    case '*': return set_substate(SYM_ASTERISK);
    case '/': return set_substate(SYM_SLASH);
    case '%': return set_substate(SYM_PERCENT);
    case '!': return set_substate(SYM_BANG);
    case '>': return set_substate(SYM_GREATER_THAN);
    case '<': return set_substate(SYM_LESS_THAN);
    case '&': return set_substate(SYM_AMPERSAND);
    case '|': return set_substate(SYM_PIPE);
    case '^': return set_substate(SYM_CARET);
    default: return set_substate(POISON);
    }
  case SYM_EQUAL:
    if (c == '>') {
      // =>
      return set_substate(SYM_STRONG_ARROW_RIGHT);
    }
    // >
    return true;
  case SYM_PLUS:
    if (c == '+') {
      // ++
      return set_substate(SYM_PLUS_PLUS);
    }
    // +
    return true;
  case SYM_MINUS:
    if (c == '-') {
      // --
      return set_substate(SYM_MINUS_MINUS);
    }
    // -
    return true;
  case SYM_SLASH:
    if (c == '/') {
      // //
      state = LexerState::COMMENT;
      return false; // Exception: The state has changed
    }
    // /
    return true;
  case SYM_BANG:
    if (c == '!') {
      // !!
      return set_substate(SYM_BANG_BANG);
    }
    // !
    return true;
  case SYM_GREATER_THAN:
    switch (c) {
    case '>': return set_substate(SYM_GREATER_THAN_GREATER_THAN); // >>
    case '=': return set_substate(SYM_GREATER_THAN_EQUAL);        // >=
    default: return true;                                         // >
    }
  case SYM_LESS_THAN:
    switch (c) {
    case '<': return set_substate(SYM_LESS_THAN_LESS_THAN); // <<
    case '=': return set_substate(SYM_LESS_THAN_EQUAL);     // <=
    default: return true;                                   // <
    }
  default: return true;
  }
}

bool Lexer::handle_number(c8 c) {
  return not(c >= '0' && c <= '9');
}

void Lexer::reset_state() {
  state = LexerState::FIND;
  token_substate = TokenType::UNKNOWN;
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