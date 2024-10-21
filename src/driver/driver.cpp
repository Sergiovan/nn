#include "driver.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

#include "frontend/parser.hpp"
#include "util/format.hpp" // IWYU pragma: keep

using namespace driver;
namespace fs = std::filesystem;

void ast_print_helper(ast::Ast& ast, std::stringstream& ss) {
  using namespace ast;
  ss << "(" << ast.get_name() << " ";
  ast.visit([&ss]<AstLike T>(T& ast) {
    if constexpr (std::is_same_v<T, AstNone>) {
      // Nothing
    } else if constexpr (std::is_same_v<T, AstToken>) {
      std::print(ss, "{}", ast.t);
    } else if constexpr (std::is_same_v<T, AstInteger>) {
      std::print(ss, "{}", ast.t);
    } else if constexpr (std::is_same_v<T, AstIdentifier>) {
      std::print(ss, "{}", ast.t);
    } else if constexpr (std::is_same_v<T, AstUnary>) {
      ast_print_helper(*ast.child, ss);
    } else if constexpr (std::is_same_v<T, AstReturn>) {
      ast_print_helper(*ast.child, ss);
    } else if constexpr (std::is_same_v<T, AstBinary>) {
      ast_print_helper(*ast.lhs, ss);
      ast_print_helper(*ast.rhs, ss);
    } else if constexpr (std::is_same_v<T, AstList>) {
      for (auto& elem : ast.asts) {
        ast_print_helper(*elem, ss);
      }
    } else if constexpr (std::is_same_v<T, AstFunction>) {
      ast_print_helper(*ast.name, ss);
      ast_print_helper(*ast.body, ss);
    }

    // Comment
    return 0;
  });
  ss << ")";
}

void lispy_print(const std::string& str) {
  s64 indent = 0;
  bool escaped = false;
  bool in_string = false;
  bool last_newline = false;
  for (auto c : str) {
    if (c == '(' && !in_string) {
      indent += 2;
      std::cout << c << '\n' << std::string(indent, ' ');
      last_newline = true;
      escaped = false;
      continue;
    } else if (c == ')' && !in_string) {
      indent -= 2;
      if (last_newline) {
        std::cout << '\b' << '\b';
      } else {
        std::cout << '\n' << std::string(indent, ' ');
      }
      std::cout << c << "\n" << std::string(indent, ' ');
      last_newline = true;
      escaped = false;
      continue;
    } else if (c == '"') {
      if (!escaped) {
        in_string = !in_string;
      }
      std::cout << c;
    } else if (c == '\\') {
      if (!escaped) {
        escaped = true;
      }
      std::cout << c;
      last_newline = false;
      continue; // Specifically skip over escaping
    } else {
      std::cout << c;
    }

    escaped = false;
    last_newline = false;
  }

  std::cout << std::flush;
}

std::string escape(std::string in) {
  std::stringstream out;
  for (char c : in) {
    switch (c) {
    case '\"':
      out << "\\\"";
      break;
    case '>':
      out << "\\>";
      break;
    case '{':
      out << "\\{";
      break;
    case '}':
      out << "\\}";
      break;
    case '\n':
      out << "\\n";
      break;
    default:
      out << c;
      break;
    }
  }
  return out.str();
}

class DotWriter {
public:
  auto to_dot(ast::Ast& ast) {
    ss << "digraph AST {\n";
    ss << "node [shape=record];\n";
    to_dot_helper(ast);
    ss << "}";

    return ss.str();
  }

private:
  u64 to_dot_helper(ast::Ast& ast) {
    using namespace ast;

    u64 elem_node = counter++;

    std::stringstream label{};
    label << "{";
    label << ast.get_name();
    label << "(" << elem_node << ")";
    label << escape(std::format("| {} ", ast.source_location().get()));

    ast.visit([this, &label, elem_node]<AstLike T>(T& ast) {
      if constexpr (std::is_same_v<T, AstNone>) {
        // Nothing
      } else if constexpr (std::is_same_v<T, AstToken>) {
        label << escape(std::format("| {}", ast.t));
      } else if constexpr (std::is_same_v<T, AstInteger>) {
        label << escape(std::format("| {}", ast.t));
      } else if constexpr (std::is_same_v<T, AstIdentifier>) {
        label << escape(std::format("| {}", ast.t));
      } else if constexpr (std::is_same_v<T, AstUnary>) {
        u64 child = to_dot_helper(*ast.child);
        std::println(ss, "{} -> {};", elem_node, child);
      } else if constexpr (std::is_same_v<T, AstReturn>) {
        u64 child = to_dot_helper(*ast.child);
        std::println(ss, "{} -> {};", elem_node, child);
      } else if constexpr (std::is_same_v<T, AstBinary>) {
        u64 lhs = to_dot_helper(*ast.lhs);
        u64 rhs = to_dot_helper(*ast.rhs);

        std::println(ss, "{} -> {};", elem_node, lhs);
        std::println(ss, "{} -> {};", elem_node, rhs);
      } else if constexpr (std::is_same_v<T, AstList>) {
        for (auto& elem : ast.asts) {
          u64 child = to_dot_helper(*elem);
          std::println(ss, "{} -> {};", elem_node, child);
        }
      } else if constexpr (std::is_same_v<T, AstFunction>) {
        u64 name = to_dot_helper(*ast.name);
        u64 body = to_dot_helper(*ast.body);
        std::println(ss, "{} -> {} [label=\"name\"];", elem_node, name);
        std::println(ss, "{} -> {} [label=\"body\"];", name, body);
      }

      // Comment
      return 0;
    });

    std::println(ss, "{} [label=\"{}}}\"];", elem_node, label.str());

    return elem_node;
  }

  u64 counter{0};
  std::stringstream ss{};
};

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
    } else if (arg == "--parse") {
      set_option(Option::StopAfterParse, true);
    } else if (arg == "--dot") {
      set_option(Option::ParseShowDot, true);
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

  if (get_option(Option::StopAfterLex)) {
    auto tokens = lexer.collect();
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
  }

  parser::Parser p{lexer};

  auto r = p.parse();

  if (get_option(Option::StopAfterParse)) {
    if (get_option(Option::ParseShowDot)) {
      DotWriter dw{};

      std::print("{}", dw.to_dot(r));
    } else {
      std::stringstream ss{};
      ast_print_helper(r, ss);
      lispy_print(ss.str());
    }

    return 0; // TODO parser.had_error()
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
  --lex: Only go up to lexing, then print the tokens
  --parse: Only go up to parsing, then print the asts
  --dot: Show parse output as a dot file
)";
  // clang-format on

  std::println("{}", help_text);
}