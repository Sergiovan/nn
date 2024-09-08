#include <filesystem>
#include <fstream>
#include <print>
#include <span>

#include "frontend/lexer.hpp"
#include "util/format.hpp" // IWYU pragma: keep

namespace fs = std::filesystem;

void print_help() {
  std::string help =
      R"(nn : Compiler for the nn language

USAGE: nn <FILE> [--help] [--lex]

  --help: Show this help
  --lex: Only go up to lexing
)";

  std::println("{}", help);
}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::println("Please provide a file to compile");
    return 1;
  }

  std::vector<std::string> args = std::span{argv + 1, argv + argc} |
                                  std::ranges::views::transform([](char* s) {
                                    return std::string{s};
                                  }) |
                                  std::ranges::to<std::vector>();

  fs::path path = "";
  bool show_help = false;
  bool lex_stop = false;

  for (auto& arg : args) {
    if (arg == "--help") {
      show_help = true;
    } else if (arg == "--lex") {
      lex_stop = true;
    } else {
      path = arg;
    }
  }

  if (show_help) {
    print_help();
    return 0;
  }

  if (!fs::exists(path)) {
    std::println("File {} does not exist", path.c_str());
    return 1;
  }

  std::ifstream file{path};
  std::string content{std::istreambuf_iterator<char>{file},
                      std::istreambuf_iterator<char>{}};

  lexer::Lexer lex;
  auto res = lex.lex(content);

  if (lex_stop) {
    std::println("[");
    u64 padding = std::to_string(res.size() - 1).length();
    for (auto [i, tok] : std::views::enumerate(res)) {
      std::println("  {:>{}}# {},", i, padding, tok);
    }
    std::println("]");

    return lex.had_error();
  } else if (lex.had_error()) {
    return 1;
  }

  return 0;
}