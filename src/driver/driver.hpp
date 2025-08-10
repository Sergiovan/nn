#pragma once

#include "frontend/lexer.hpp"
#include "transform/asm_parser.hpp"
#include "util/enum_map.hpp"

namespace driver {

enum class Option {
  /** Show help for the compiler arguments */
  ShowHelp,
  /** Stop processing the source after the Lexing phase */
  StopAfterLex,
  /** Stop processing the source after the parsing phase */
  StopAfterParse,
  /** Stop processing the source after the tac phase */
  StopAfterTac,
  /** Stop processing the source after the codegen phase */
  StopAfterCodegen,
  /** Instead of assmbling and linking an executable, dump a file
      with the generated assembly */
  EmitAsmFile,
  /** Instead of assembling and linking an executable, print asm to stdout */
  PrintAsmFile,
  /** Show dotfile syntax of the AST parsed */
  ParseShowDot,
  /** Do not print secondary output to stdout */
  Silent,

  LAST,
  FIRST = ShowHelp
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
  int32_t finish_compilation(asm_parser::ParseResult program);

  /** Creates a lexer from a filename and error manager. Will open and read the file */
  lexer::Lexer get_lexer(const std::string& filename,
                         error::ErrorManager& error_manager);

  /** Print the compiler help */
  void print_help();

  /** Bitmap of compiler boolean options */
  EnumBitMap<Option> options{};
  /** Entry point source file to compile */
  std::string entry_point{};
  /** Output file name */
  std::string output_file{"out"};
};

} // namespace driver