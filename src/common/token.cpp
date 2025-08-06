#include "common/token.hpp"

using namespace token;

const char* token::name(TokenType tt) {
  switch (tt) {
    using enum TokenType;
  case UNKNOWN: return "KNOWN_UNKNOWN";
  case POISON: return "POISON";
  case END: return "END";

  case WHITESPACE: return "WHITESPACE";
  case COMMENT: return "COMMENT";

  case IDENTIFIER: return "IDENTIFIER";
  case INTEGER: return "INTEGER";

  case KW_DEF: return "KW_DEF";
  case KW_FUN: return "KW_FUN";
  case KW_RETURN: return "KW_RETURN";

  case SYM_OPEN_PAREN: return "SYM_OPEN_PAREN";
  case SYM_CLOSE_PAREN: return "SYM_CLOSE_PAREN";
  case SYM_OPEN_BRACE: return "SYM_OPEN_BRACE";
  case SYM_CLOSE_BRACE: return "SYM_CLOSE_BRACE";
  case SYM_SEMICOLON: return "SYM_SEMICOLON";
  case SYM_EQUAL: return "SYM_EQUAL";
  case SYM_STRONG_ARROW_RIGHT: return "SYM_STRONG_ARROW_RIGHT";
  case SYM_PLUS: return "SYM_PLUS";
  case SYM_PLUS_PLUS: return "SYM_PLUS_PLUS";
  case SYM_MINUS: return "SYM_MINUS";
  case SYM_MINUS_MINUS: return "SYM_MINUS_MINUS";
  case SYM_ASTERISK: return "SYM_ASTERISK";
  case SYM_SLASH: return "SYM_SLASH";
  case SYM_PERCENT: return "SYM_PERCENT";
  case SYM_BANG: return "SYM_BANG";
  case SYM_BANG_BANG: return "SYM_BANG_BANG";
  case SYM_GREATER_THAN: return "SYM_GREATER_THAN";
  case SYM_GREATER_THAN_GREATER_THAN: return "SYM_GREATER_THAN_GREATER_THAN";
  case SYM_GREATER_THAN_EQUAL: return "SYM_GREATER_THAN_EQUAL";
  case SYM_LESS_THAN: return "SYM_LESS_THAN";
  case SYM_LESS_THAN_LESS_THAN: return "SYM_LESS_THAN_LESS_THAN";
  case SYM_LESS_THAN_EQUAL: return "SYM_LESS_THAN_EQUAL";
  case SYM_AMPERSAND: return "SYM_AMPERSAND";
  case SYM_PIPE: return "SYM_PIPE";
  case SYM_CARET: return "SYM_CARET";

  case LAST: return "UNKNOWN_UNKNOWN";
  }
}

const char* token::as_source(TokenType tt) {
  switch (tt) {
    using enum TokenType;
  case UNKNOWN: return "<?>";
  case POISON: return "<!>";
  case END: return "<0>";

  case WHITESPACE: return "< >";
  case COMMENT: return "<//>";

  case IDENTIFIER: return "<idn>";
  case INTEGER: return "<integer>";

  case KW_DEF: return "def";
  case KW_FUN: return "fun";
  case KW_RETURN: return "return";

  case SYM_OPEN_PAREN: return "(";
  case SYM_CLOSE_PAREN: return ")";
  case SYM_OPEN_BRACE: return "{";
  case SYM_CLOSE_BRACE: return "}";
  case SYM_SEMICOLON: return ";";
  case SYM_EQUAL: return "=";
  case SYM_STRONG_ARROW_RIGHT: return "=>";
  case SYM_PLUS: return "+";
  case SYM_PLUS_PLUS: return "++";
  case SYM_MINUS: return "-";
  case SYM_MINUS_MINUS: return "--";
  case SYM_ASTERISK: return "*";
  case SYM_SLASH: return "/";
  case SYM_PERCENT: return "%";
  case SYM_BANG: return "!";
  case SYM_BANG_BANG: return "!!";
  case SYM_GREATER_THAN: return ">";
  case SYM_GREATER_THAN_GREATER_THAN: return ">>";
  case SYM_GREATER_THAN_EQUAL: return ">=";
  case SYM_LESS_THAN: return "<";
  case SYM_LESS_THAN_LESS_THAN: return "<<";
  case SYM_LESS_THAN_EQUAL: return "<=";
  case SYM_AMPERSAND: return "&";
  case SYM_PIPE: return "|";
  case SYM_CARET: return "^";

  case LAST: return "<?\?>";
  }
}