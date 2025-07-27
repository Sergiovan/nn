#pragma once
#include <print> // IWYU pragma: keep
#include <type_traits>

#include "common/source.hpp"
#include "util/types.hpp"

namespace token {

/** Type of a single token from a source file */
enum class TokenType : u16 {
  /* An invalid, unknown token */
  UNKNOWN,
  /* An invalid token that propagats an error */
  POISON,
  /* A token marking the end of input/source */
  END,

  /* A token containing whitespace */
  WHITESPACE,
  /* A token containing a comment */
  COMMENT,
  /* A token containing a block comment */
  // COMMENT_BLOCK,

  /* A token containing an identifier */
  IDENTIFIER,
  /* A token contianing an integer */
  INTEGER,

  /* Keyword token `def` */
  KW_DEF,
  /* Keyword token `fun` */
  KW_FUN,
  /* Keywork token `return` */
  KW_RETURN,

  /* Symbol token `+` */
  // SYM_PLUS,
  /* Symbol token `-` */
  SYM_MINUS,
  /* Symbol token `--` */
  SYM_MINUS_MINUS,
  /* Symbol token `!` */
  SYM_BANG,
  /* Symbol token `!!` */
  SYM_BANG_BANG,
  /* Symbol token `=>` */
  SYM_STRONG_ARROW_RIGHT,
  /* Symbol token `(` */
  SYM_OPEN_PAREN,
  /* Symbol token `)` */
  SYM_CLOSE_PAREN,
  /* Symbol token `{` */
  SYM_OPEN_BRACE,
  /* Symbol token `}` */
  SYM_CLOSE_BRACE,

  /* Symbol token `;` */
  SYM_SEMICOLON,
};

/** Represents a single token of the source code */
struct Token {
  TokenType tt;
  source::SourceLocation loc;
};

} // namespace token

template <>
struct std::formatter<token::TokenType> : std::formatter<std::string_view> {
  auto format(const token::TokenType& tok, std::format_context& ctx) const {
    std::string name = "UNKNOWN_UNKNOWN";
    switch (tok) {
      using enum token::TokenType;
    case UNKNOWN:
      name = "KNOWN_UNKNOWN";
      break;
    case POISON:
      name = "POISON";
      break;
    case END:
      name = "END";
      break;
    case WHITESPACE:
      name = "WHITESPACE";
      break;
    case COMMENT:
      name = "COMMENT";
      break;
    case IDENTIFIER:
      name = "IDENTIFIER";
      break;
    case INTEGER:
      name = "INTEGER";
      break;
    case KW_DEF:
      name = "KW_DEF";
      break;
    case KW_FUN:
      name = "KW_FUN";
      break;
    case KW_RETURN:
      name = "KW_RETURN";
      break;
    case SYM_MINUS:
      name = "SYM_MINUS";
      break;
    case SYM_MINUS_MINUS:
      name = "SYM_MINUS_MINUS";
      break;
    case SYM_BANG:
      name = "SYM_BANG";
      break;
    case SYM_BANG_BANG:
      name = "SYM_BANG_BANG";
      break;
    case SYM_STRONG_ARROW_RIGHT:
      name = "SYM_STRONG_ARROW_RIGHT";
      break;
    case SYM_OPEN_PAREN:
      name = "SYM_OPEN_PAREN";
      break;
    case SYM_CLOSE_PAREN:
      name = "SYM_CLOSE_PAREN";
      break;
    case SYM_OPEN_BRACE:
      name = "SYM_OPEN_BRACE";
      break;
    case SYM_CLOSE_BRACE:
      name = "SYM_CLOSE_BRACE";
      break;
    case SYM_SEMICOLON:
      name = "SYM_SEMICOLON";
      break;
    }

    return std::formatter<std::string_view>::format(name, ctx);
  }
};

template <>
struct std::formatter<token::Token> {
  constexpr auto parse(std::format_parse_context& ctx) {
    return ctx.begin();
  }

  auto format(const token::Token& tok, std::format_context& ctx) const {
    std::format_to(
        ctx.out(), "{} [{}]: ", tok.tt,
        static_cast<std::underlying_type_t<decltype(tok.tt)>>(tok.tt));

    std::string value = tok.loc.get();

    if (auto newline = value.find('\n'); newline < 32) {
      newline = std::min(newline, 29uz);
      return std::format_to(ctx.out(), "\"{}\\n*\"", value.substr(0, newline));
    } else if (value.length() > 32) {
      return std::format_to(ctx.out(), "\"{}...\"", value.substr(0, 29));
    } else {
      return std::format_to(ctx.out(), "\"{}\"", value);
    }
  }
};