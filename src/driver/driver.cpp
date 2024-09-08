#include "driver.hpp"

#include <filesystem>
#include <fstream>

#include "util/format.hpp" // IWYU pragma: keep

using namespace driver;
namespace fs = std::filesystem;

Driver::Driver(int argc, char** argv) {
  std::vector<std::string_view> args =
      std::span{argv + 1, argv + argc} |
      std::ranges::views::transform([](char* s) {
        return std::string_view{s};
      }) |
      std::ranges::to<std::vector>();

  for (auto& arg : args) {
    if (arg == "--help") {
      set_option(Option::ShowHelp, true);
    } else if (arg == "--lex") {
      set_option(Option::StopAfterLex, true);
    } else {
      entry_point = arg;
    }
  }
}

int Driver::run() {
  if (get_option(Option::ShowHelp)) {
    print_help();
    return 0;
  }

  if (!fs::exists(entry_point)) {
    std::println("File {} does not exist", entry_point);
    return 1;
  }

  auto lexer = get_lexer(entry_point);
  auto tokens = lexer.collect();

  if (get_option(Option::StopAfterLex)) {
    std::println("[  LEN: {}", tokens.size());
    u64 padding = std::to_string(tokens.size() - 1).length();
    for (auto [i, tok] : std::views::enumerate(tokens)) {
      if (tok.tt == token::TokenType::WHITESPACE) {
        continue;
      }

      std::println("  {:>{}}# {},", i, padding, tok);
    }
    std::println("]");

    return lexer.had_error();
  } else if (lexer.had_error()) {
    return 1;
  }

  return 0;
}

void Driver::set_option(Option option, bool value) {
  options.set(option, value);
}

bool Driver::get_option(Option option) {
  return options[option];
}

lexer::Lexer Driver::get_lexer(const std::string& filename) {
  std::ifstream file{filename};
  std::string content{std::istreambuf_iterator<char>{file},
                      std::istreambuf_iterator<char>{}};

  lexer::Lexer lex{content};

  return lex;
}

void Driver::print_help() {
  // TODO Move to another file
  // clang-format off
constexpr char help_text[] =
R"(nn : Compiler for the nn language

USAGE: nn <FILE> [--help] [--lex]

  --help: Show this help
  --lex: Only go up to lexing
)";
  // clang-format on

  std::println("{}", help_text);
}