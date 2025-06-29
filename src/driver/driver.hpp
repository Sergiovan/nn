#pragma once

#include "frontend/lexer.hpp"
#include "util/enum_map.hpp"

namespace driver {

enum class Option {
  /** Show help for the compiler arguments */
  ShowHelp,
  /** Stop processing the source after the Lexing phase */
  StopAfterLex,
  /** Stop processing the source after the parsing phase */
  StopAfterParse,
  /** Show dotfile syntax of the AST parsed */
  ParseShowDot,

  LAST
};

class Driver {
public:
  /** Entry point for the compiler. Parses args from argv but does not compile */
  Driver(int argc, char** argv);

  /** Compiles the file passed through argv */
  int run();

  /** Sets a compiler boolean option */
  void set_option(Option option, bool value);
  /** Gets the value of a compiler boolean option */
  bool get_option(Option option);

private:
  /** Creates a lexer from a filename and error manager. Will open and read the file */
  lexer::Lexer get_lexer(const std::string& filename,
                         error::ErrorManager& error_manager);

  /** Print the compiler help */
  void print_help();

  /** Bitmap of compiler boolean options */
  EnumBitMap<Option> options{};
  /** Entry point source file to compile */
  std::string entry_point{};
};

} // namespace driver