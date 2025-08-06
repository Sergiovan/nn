#pragma once

#include <string_view>
#include <vector>

#include "common/error.hpp"
#include "common/source.hpp"
#include "common/token.hpp"

#include "util/types.hpp"

namespace lexer {

/** Converts a source file into separate tokens */
class Lexer {
public:
  /** Creates a lexer for the given source */
  Lexer(std::shared_ptr<source::Source> content,
        error::ErrorManager& error_manager);

  /** Gets the next token and advances the internal state */
  token::Token next();
  /** Collects all tokens into a vector. After this runs, `next()` will 
      always return an END token */
  std::vector<token::Token> collect();
  /** If parsing this source generated any error */
  bool had_error();

private:
  /** Current state of the Lexer lookup */
  enum class LexerState {
    /* The state is yet to be determined */
    FIND,

    /* Lexing whitespace */
    WHITESPACE,
    /* Lexing a comment */
    COMMENT,
    /* Lexing a block comment */
    // BLOCK_COMMENT,

    /* Lexing an identifier */
    IDENTIFIER,
    /* Lexing a symbol */
    SYMBOL,

    /* Lexing some sort of number */
    NUMBER,
    /* Lexing an integer */
    INTEGER,
    /* Lexing a string */
    // STRING,

    /* An invalid character was found */
    ERROR,
    /* The lexer has reached the end of the source */
    END,
  };

  /* The handle_ family of functions handles a specific state of the lexer. 
     As input they take the current character being read. Internally they change the
     state and substate of the lexer. The return value is true if the end of the state
     has been reached (i.e. the state has to be exited), false otherwise*/

  /* Handles the FIND state. The next state is determined based on the 
     current character */
  void handle_find(c8 c);

  /* Handles the WHITESPACE state. All contiguous whitespace is merged into 
     one token */
  bool handle_whitespace(c8 c);
  /* Handles the COMMENT state. Reads single-line comments to the end of the line 
     or the end of input */
  bool handle_comment(c8 c);
  /* Handles the BLOCK_COMMENT state. Reads a block comment until the end */
  // bool handle_block_comment(c8 c);
  /* Handles the IDENTIFIER state. Accepts [a-zA-Z_] for now */
  bool handle_identifier(c8 c);
  /* Handles the SYMBOL state. Reads all valid symbols */
  bool handle_symbol(c8 c);
  /* Handles the NUMBER state. Reads any sort of number. 
     TODO Disambiguate integer or rational */
  bool handle_number(c8 c);

  /* Resets the state and substate to FIND and NONE respectively */
  void reset_state();
  /* Consumes the current character and advances. Takes care of updating
     the current line and column */
  void advance_char();

  /* Creates a SourceLocation for the current token */
  source::SourceLocation current_loc();

  /* The source as a shared pointer */
  std::shared_ptr<source::Source> source;
  /* String view of the source text */
  std::string_view content;

  /* Error manager, for token errors */
  error::ErrorManager& error_manager;

  /* Current absolute position of the beginning of the token being processed */
  u32 current_token_start = 0;
  /* Line of the beginning of the token being processed */
  u32 token_start_line = 0;
  /* Column of the beginning of the token being processed */
  u32 token_start_col = 0;

  /* Index of the character currently being read */
  u32 current_pos = 0;
  /* Currentl 0-indexed line in the source */
  u32 current_line = 0;
  /* Current 0-indexed column in the source */
  u32 current_col = 0;
  /* Current character being processed */
  c8 current_char = '\0';

  /* Current lexer state */
  LexerState state = LexerState::FIND;
  /* Current lexer substate */
  token::TokenType token_substate = token::TokenType::UNKNOWN;
  /* Level of block comment recursion */
  u32 comment_recursion = 0;

  /* If any error has occurred while lexing this source file */
  bool _had_error = false;
  /* If this source file has finished lexing */
  bool finished = false;
};

} // namespace lexer