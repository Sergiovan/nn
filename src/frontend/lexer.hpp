#pragma once

#include <string_view>
#include <vector>

#include "common/source.hpp"
#include "common/token.hpp"

#include "util/types.hpp"

namespace lexer {

class Lexer {
public:
  Lexer(const std::string& content);

  token::Token next();
  std::vector<token::Token> collect();
  bool had_error();

private:
  enum class LexerState {
    FIND,

    WHITESPACE,
    COMMENT,
    // BLOCK_COMMENT,

    IDENTIFIER,
    SYMBOL,

    NUMBER,
    INTEGER,
    // STRING,

    ERROR,
    END,
  };

  enum class LexerSubState {
    NONE,

    SYMBOL_EQUAL,
    // SYMBOL_FORWARD_SLASH,
    SYMBOL_DONE,
  };

  void handle_find(c8 c);

  bool handle_whitespace(c8 c);
  bool handle_comment(c8 c);
  // bool handle_block_comment(c8 c);
  bool handle_identifier(c8 c);
  bool handle_symbol(c8 c);
  bool handle_number(c8 c);

  void reset_state();
  void advance_char();
  source::SourceLocation current_loc();

  std::shared_ptr<source::Source> source;
  std::string_view content;

  u32 current_token_start = 0;
  u32 token_start_line = 0;
  u32 token_start_col = 0;

  u32 current_pos = 0;
  u32 current_line = 0;
  u32 current_col = 0;
  c8 current_char = '\0';

  LexerState state = LexerState::FIND;
  LexerSubState substate = LexerSubState::NONE;
  u32 comment_recursion = 0;

  bool _had_error = false;
  bool finished = false;
};

} // namespace lexer