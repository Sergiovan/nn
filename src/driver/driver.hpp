#pragma once

#include "frontend/lexer.hpp"
#include "util/enum_map.hpp"

namespace driver {

enum class Option {
  ShowHelp,
  StopAfterLex,
  StopAfterParse,
  ParseShowDot,

  LAST
};

class Driver {
public:
  Driver(int argc, char** argv);

  int run();

  void set_option(Option option, bool value);
  bool get_option(Option option);

private:
  lexer::Lexer get_lexer(const std::string& filename,
                         error::ErrorManager& error_manager);

  void print_help();

  EnumBitMap<Option> options{};
  std::string entry_point{};
};

} // namespace driver