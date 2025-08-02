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

  /* LAST */
  LAST,
};

/** Represents a single token of the source code */
struct Token {
  TokenType tt;
  source::SourceLocation loc;
};

const char* name(TokenType tt);
const char* as_source(TokenType tt);

} // namespace token

template <>
struct std::formatter<token::TokenType>
    : std::formatter<std::string_view>,
      std::formatter<std::underlying_type_t<token::TokenType>> {

  using Underlying = std::underlying_type_t<token::TokenType>;
  bool as_number = false;
  bool as_source = false;

  constexpr auto parse(std::format_parse_context& ctx) {
    constexpr const char AS_SOURCE[] = "source";
    constexpr const char AS_NUMBER[] = "number";
    std::string_view ctx_str{ctx};

    if (ctx_str.starts_with(AS_SOURCE)) {
      as_source = true;
      ctx.advance_to(ctx.begin() + sizeof(AS_SOURCE) - 1);
      return std::formatter<std::string_view>::parse(ctx);

    } else if (ctx_str.starts_with(AS_NUMBER)) {
      as_number = true;
      ctx.advance_to(ctx.begin() + sizeof(AS_NUMBER) - 1);
      return std::formatter<Underlying>::parse(ctx);
    } else {
      return std::formatter<std::string_view>::parse(ctx);
    }
  }

  auto format(const token::TokenType& tok, std::format_context& ctx) const {
    if (as_number) {
      return std::formatter<Underlying>::format(static_cast<Underlying>(tok),
                                                ctx);
    }

    return std::formatter<std::string_view>::format(
        as_source ? token::as_source(tok) : token::name(tok), ctx);
  }
};

template <>
struct std::formatter<token::Token> {
  constexpr auto parse(std::format_parse_context& ctx) {
    return ctx.begin();
  }

  auto format(const token::Token& tok, std::format_context& ctx) const {
    std::format_to(ctx.out(), "{} [{:number}]: ", tok.tt, tok.tt);

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