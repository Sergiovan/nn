#pragma once
#include <print>
#include <span>
#include <type_traits>

#include "util/types.hpp"

namespace token {

enum class TokenType : u16 {
  POISON,
  END,

  WHITESPACE,
  COMMENT,
  // COMMENT_BLOCK,

  IDENTIFIER,
  INTEGER,

  KW_DEF,
  KW_FUN,
  KW_RETURN,

  // SYM_PLUS,
  SYM_STRONG_ARROW_RIGHT,
  SYM_OPEN_PAREN,
  SYM_CLOSE_PAREN,
  SYM_OPEN_BRACE,
  SYM_CLOSE_BRACE,

  SYM_SEMICOLON,
};

struct Token {
  TokenType tt;
  std::span<const c8> span;
};

} // namespace token

template <>
struct std::formatter<token::TokenType> : std::formatter<std::string_view> {
  auto format(const token::TokenType& tok, std::format_context& ctx) const {
    std::string name = "";
    switch (tok) {
    case token::TokenType::POISON:
      name = "POISON";
      break;
    case token::TokenType::END:
      name = "END";
      break;
    case token::TokenType::WHITESPACE:
      name = "WHITESPACE";
      break;
    case token::TokenType::COMMENT:
      name = "COMMENT";
      break;
    case token::TokenType::IDENTIFIER:
      name = "IDENTIFIER";
      break;
    case token::TokenType::INTEGER:
      name = "INTEGER";
      break;
    case token::TokenType::KW_DEF:
      name = "KW_DEF";
      break;
    case token::TokenType::KW_FUN:
      name = "KW_FUN";
      break;
    case token::TokenType::KW_RETURN:
      name = "KW_RETURN";
      break;
    case token::TokenType::SYM_STRONG_ARROW_RIGHT:
      name = "SYM_STRONG_ARROW_RIGHT";
      break;
    case token::TokenType::SYM_OPEN_PAREN:
      name = "SYM_OPEN_PAREN";
      break;
    case token::TokenType::SYM_CLOSE_PAREN:
      name = "SYM_CLOSE_PAREN";
      break;
    case token::TokenType::SYM_OPEN_BRACE:
      name = "SYM_OPEN_BRACE";
      break;
    case token::TokenType::SYM_CLOSE_BRACE:
      name = "SYM_CLOSE_BRACE";
      break;
    case token::TokenType::SYM_SEMICOLON:
      name = "SYM_SEMICOLON";
      break;
    }

    return std::formatter<std::string_view>::format(name, ctx);
  }
};

template <> struct std::formatter<token::Token> {
  constexpr auto parse(std::format_parse_context& ctx) {
    return ctx.begin();
  }

  auto format(const token::Token& tok, std::format_context& ctx) const {
    std::string_view sv{tok.span};
    std::format_to(
        ctx.out(), "{} ({}): ", tok.tt,
        static_cast<std::underlying_type_t<decltype(tok.tt)>>(tok.tt));

    if (auto newline = sv.find('\n'); newline < 32) {
      newline = std::min(newline, 29uz);
      return std::format_to(ctx.out(), "\"{}\\n*\"", sv.substr(0, newline));
    } else if (sv.length() > 32) {
      return std::format_to(ctx.out(), "\"{}...\"", sv.substr(0, 29));
    } else {
      return std::format_to(ctx.out(), "\"{}\"", sv);
    }
  }
};