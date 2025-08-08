#include "driver.hpp"

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <ranges>

#include <sys/wait.h>
#include <unistd.h>

#include "backend/output_gnu_as.hpp"
#include "common/asm_ast.hpp"
#include "common/ast.hpp"
#include "common/error.hpp"
#include "common/tac.hpp"
#include "frontend/parser.hpp"
#include "transform/asm_parser.hpp"
#include "transform/tac_parser.hpp"
#include "util/format.hpp" // IWYU pragma: keep
#include "util/scope_guard.hpp"

using namespace driver;
namespace fs = std::filesystem;

/** Prints an AST `ast` into `ss`. Recursively prints all ASTs it finds inside */
void ast_print_helper(const ast::Ast& ast, const ast::AstContainer& container,
                      std::stringstream& ss) {
  using namespace ast;
  ss << "(" << ast.get_name() << " ";
  ScopeGuard sg = [&ss]() {
    ss << ")";
  };

  switch (ast.get_tag()) {
    using enum ast::Tag;
  case NONE: // Nothing;
    return;
  case INTEGER: std::print(ss, "{}", ast.get<INTEGER>().t); return;
  case IDENTIFIER: std::print(ss, "{}", ast.get<IDENTIFIER>().t); return;
  case RETURN:
    ast_print_helper(ast.get<RETURN>().child.from(container), container, ss);
    return;
  case PRE_OP: {
    auto& pre_op = ast.get<PRE_OP>();
    std::print(ss, "{} ", pre_op.t);
    ast_print_helper(pre_op.child.from(container), container, ss);
    return;
  }
  case LIST:
    for (const auto& elem : ast.get<LIST>().asts) {
      ast_print_helper(elem.from(container), container, ss);
    }
    return;
  case FUNCTION: {
    auto& fn = ast.get<FUNCTION>();
    ast_print_helper(fn.name.from(container), container, ss);
    ast_print_helper(fn.body.from(container), container, ss);
    return;
  }
  case LAST:
  }
  unreachable;
}

/** Prints the output of `ast_print_helper` in a "readable" manner */
void lispy_print(std::ostream& os, const std::string& str) {
  s64 indent = 0;
  bool escaped = false;
  bool in_string = false;
  bool last_newline = false;
  for (auto c : str) {
    if (c == '(' && !in_string) {
      indent += 2;
      os << c << '\n' << std::string(indent, ' ');
      last_newline = true;
      escaped = false;
      continue;
    } else if (c == ')' && !in_string) {
      indent -= 2;
      if (last_newline) {
        os << '\b' << '\b';
      } else {
        os << '\n' << std::string(indent, ' ');
      }
      os << c << "\n" << std::string(indent, ' ');
      last_newline = true;
      escaped = false;
      continue;
    } else if (c == '"') {
      if (!escaped) {
        in_string = !in_string;
      }
      os << c;
    } else if (c == '\\') {
      if (!escaped) {
        escaped = true;
      }
      os << c;
      last_newline = false;
      continue; // Specifically skip over escaping
    } else {
      os << c;
    }

    escaped = false;
    last_newline = false;
  }

  os << std::flush;
}

/** Escapes a string for dot format */
std::string escape_dot(std::string in) {
  std::stringstream out;
  for (char c : in) {
    switch (c) {
    case '\"': out << "\\\""; break;
    case '>': out << "\\>"; break;
    case '{': out << "\\{"; break;
    case '}': out << "\\}"; break;
    case '\n': out << "\\n"; break;
    default: out << c; break;
    }
  }
  return out.str();
}

/** Converts an AST into a dot format string */
class DotWriter {
public:
  /** Converts ast `ast` into a dot format string */
  auto to_dot(const ast::Ast& ast, const ast::AstContainer& container) {
    ss << "digraph AST {\n";
    ss << "node [shape=record];\n";
    to_dot_helper(ast, container);
    ss << "}";

    return ss.str();
  }

private:
  /** Recursively converst the given ast into dot format strings.
      Returns the dot ID of the ast it parsed */
  u64 to_dot_helper(const ast::Ast& ast, const ast::AstContainer& container) {
    using namespace ast;

    u64 elem_node = counter++;

    std::stringstream label{};
    label << "{";
    label << ast.get_name();
    label << "(" << elem_node << ")";
    label << escape_dot(
        std::format("| {} ", ast.source_location(container).get()));

    ScopeGuard sg = [this, elem_node, &label]() {
      std::println(ss, "{} [label=\"{}}}\"];", elem_node, label.str());
    };

    switch (ast.get_tag()) {
      using enum ast::Tag;
    case NONE: // Nothing
      return elem_node;
    case INTEGER:
      label << escape_dot(std::format("| {}", ast.get<INTEGER>().t));
      return elem_node;
    case IDENTIFIER:
      label << escape_dot(std::format("| {}", ast.get<IDENTIFIER>().t));
      return elem_node;
    case RETURN: {
      auto& ret = ast.get<RETURN>();
      u64 child = to_dot_helper(ret.child.from(container), container);
      std::println(ss, "{} -> {};", elem_node, child);
      return elem_node;
    }
    case PRE_OP: {
      auto& pre_op = ast.get<PRE_OP>();
      u64 child = to_dot_helper(pre_op.child.from(container), container);
      std::println(ss, "{} -> {};", elem_node, child);
      return elem_node;
    }
    case LIST: {
      auto& list = ast.get<LIST>();
      for (const auto& elem : container.iterate_content(list.asts)) {
        u64 child = to_dot_helper(elem, container);
        std::println(ss, "{} -> {};", elem_node, child);
      }
      return elem_node;
    }
    case FUNCTION: {
      auto& fn = ast.get<FUNCTION>();
      u64 name = to_dot_helper(fn.name.from(container), container);
      u64 body = to_dot_helper(fn.body.from(container), container);
      std::println(ss, "{} -> {} [label=\"name\"];", elem_node, name);
      std::println(ss, "{} -> {} [label=\"body\"];", name, body);
      return elem_node;
    }
    case LAST:
    }

    unreachable;
  }

  u64 counter{0};
  std::stringstream ss{};
};

void print_tac_helper(std::ostringstream& ss, tac::TacIndex idx,
                      const tac::Tac& tac, const tac::TacContainer& container);

void print_tac_helper(std::ostringstream& ss, tac::TacIndex index,
                      const tac::TacContainer& container) {
  print_tac_helper(ss, index, index.from(container), container);
}

void print_tac_helper(std::ostringstream& ss, tac::TacIndex idx,
                      const tac::Tac& tac, const tac::TacContainer& container) {
  switch (tac.get_tag()) {
    using enum tac::Tag;
  case PROGRAM: {
    const auto& prog = tac.get<PROGRAM>();
    std::println(ss, "{} = PROGRAM", idx);
    print_tac_helper(ss, prog.function, container);
  } break;
  case FUNCTION: {
    const auto& fn = tac.get<FUNCTION>();
    std::println(ss, "{} = BEGIN FUNCTION {}", idx, fn.name);
    for (auto& inst : fn.instructions) {
      print_tac_helper(ss, inst, container);
    }
    std::println(ss, "END FUNCTION {}", fn.name);
  } break;
  case RETURN: {
    const auto& ret = tac.get<RETURN>();
    print_tac_helper(ss, ret.value, container);
    std::println(ss, "{} = return {}", idx, ret.value);
  } break;
  case UNARY: {
    const auto& un = tac.get<UNARY>();
    print_tac_helper(ss, un.source, container);
    std::println(ss, "{} = {:source} {}", idx, un.op, un.source);
  } break;
  case CONSTANT: {
    const auto& constant = tac.get<CONSTANT>();
    std::println(ss, "{} = CONSTANT {}", idx, constant.value);
  } break;
  case VAR: {
    // Purposefully empty: vars are just the number
  } break;
  case IDENTIFIER: {
    const auto& iden = tac.get<IDENTIFIER>();
    std::println(ss, "{} = IDENTIFIER {}", idx, iden.iden);
  } break;
  case LAST: std::println(ss, "{} = INVALID (LAST)", idx); break;
  }
}

void print_tac(tac_parser::ParseResult tac) {
  std::ostringstream ss;

  print_tac_helper(ss, {tac.container.size() - 1}, tac.top, tac.container);

  std::print(std::cerr, "{}", ss.str());
}

std::string print_op(const asm_ast::Operand& op) {
  switch (op.get_tag()) {
    using enum asm_ast::OperandTag;
  case IMMEDIATE: return std::format("{:#x}", op.get<IMMEDIATE>().value);
  case REGISTER: return std::format("{}", op.get<REGISTER>().reg);
  case PSEUDO: return std::format("PSEUDO {}", op.get<PSEUDO>().pseudo_value);
  case STACK_ADDR: {
    auto& stck = op.get<STACK_ADDR>();
    if (stck.offset) {
      return std::format("{:#x}({})", op.get<STACK_ADDR>().offset,
                         asm_ast::Register::SP);
    } else {
      return std::format("({})", asm_ast::Register::SP);
    }
  }
  case LAST: return "OPERAND LAST (?\?)";
  }
}

std::string print_instruction(const asm_ast::Instruction& instr,
                              const asm_ast::OperandContainer& ops) {
  switch (instr.get_tag()) {
    using enum asm_ast::InstructionTag;
  case LOAD: {
    const auto& ld = instr.get<LOAD>();
    return std::format("{} {} <- {}", LOAD, print_op(ld.destination.from(ops)),
                       print_op(ld.source.from(ops)));
  }
  case STORE: {
    const auto& st = instr.get<STORE>();
    return std::format("{} {} <- {}", STORE, print_op(st.destination.from(ops)),
                       print_op(st.source.from(ops)));
  }
  case UNARY: {
    const auto& un = instr.get<UNARY>();
    return std::format("{} {} <- {}", un.op, print_op(un.destination.from(ops)),
                       print_op(un.arg.from(ops)));
  }
  case ALLOCA: {
    return std::format("STACK ALLOC {:#x}", instr.get<ALLOCA>().bytes);
  }
  case RET: {
    return "RET";
  }
  case LAST: return "INSTRUCTION LAST (?\?)";
  }
}

Driver::Driver(int argc, char** argv) {
  std::vector<std::string_view> args =
      std::span{argv + 1, argv + argc} |
      std::ranges::views::transform([](char* s) {
        return std::string_view{s};
      }) |
      std::ranges::to<std::vector>();

  enum class OptionState { NONE, OUTPUT } option_state = OptionState::NONE;

  for (auto& arg : args) {
    if (arg == "--help") {
      set_option(Option::ShowHelp, true);
    } else if (arg == "--lex") {
      set_option(Option::StopAfterLex, true);
    } else if (arg == "--parse") {
      set_option(Option::StopAfterParse, true);
    } else if (arg == "--dot") {
      set_option(Option::ParseShowDot, true);
    } else if (arg == "--tac") {
      set_option(Option::StopAfterTac, true);
    } else if (arg == "--codegen") {
      set_option(Option::StopAfterCodegen, true);
    } else if (arg == "--asm") {
      set_option(Option::PrintAsmFile, true);
    } else if (arg == "--silent") {
      set_option(Option::Silent, true);
    } else if (arg == "--output" || arg == "-o") {
      // Not particularly robust, but that's okay for now
      option_state = OptionState::OUTPUT;
    } else {
      switch (option_state) {
      case OptionState::NONE: entry_point = arg; break;
      case OptionState::OUTPUT:
        output_file = arg;
        option_state = OptionState::NONE;
        break;
      }
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

  bool silent = get_option(Option::Silent);

  auto error_manager = error::ErrorManager{};

  auto lexer = get_lexer(entry_point, error_manager);

  if (get_option(Option::StopAfterLex)) {
    auto tokens = lexer.collect();
    if (!silent) {
      std::println(std::cerr, "[ LEN: {}", tokens.size());
      u64 padding = std::to_string(tokens.size() - 1).length();
      for (auto [i, tok] : std::views::enumerate(tokens)) {
        if (tok.tt == token::TokenType::WHITESPACE) {
          continue;
        }

        std::cerr << "  " << std::setw(static_cast<int>(padding)) << std::right
                  << i << "# " << std::format("{}", tok) << ",\n";
        // See assert.hpp for why this line is off
        // std::println(std::cerr, "  {:>{}}# {},", i, padding, tok);
      }
      std::println(std::cerr, "]");
    }

    if (lexer.had_error()) {
      error_manager.print_all_simple();
      return 1;
    }

    return 0;
  }

  parser::Parser p{lexer, error_manager};

  auto parse_result = p.parse();
  auto [ast, container] = parse_result;

  if (get_option(Option::StopAfterParse) && !silent) {
    if (get_option(Option::ParseShowDot)) {
      DotWriter dw{};

      std::print(std::cerr, "{}", dw.to_dot(ast, container));
    } else {
      std::stringstream ss{};
      ast_print_helper(ast, container, ss);
      lispy_print(std::cerr, ss.str());
    }
  }

  if (error_manager.has_errors()) {
    error_manager.print_all_simple();
    return lexer.had_error() ? 1 : 2;
  }

  if (get_option(Option::StopAfterParse)) {
    return 0;
  }

  tac_parser::TacParser tp{parse_result, error_manager};

  auto tac_result = tp.parse();

  if (get_option(Option::StopAfterTac)) {
    print_tac(tac_result);
  }

  if (error_manager.has_errors()) {
    return 3;
  }

  if (get_option(Option::StopAfterTac)) {
    return 0;
  }

  auto [tac, tac_container] = tac_result;

  asm_parser::AsmParser ap{tac, tac_container};

  auto asm_result = ap.parse();
  auto [asm_, asm_instructions, asm_operands] = asm_result;

  if (get_option(Option::StopAfterCodegen)) {
    if (!silent) {
      std::println("{}:", asm_.fn.name);
      for (const auto [idx, instr] : std::views::enumerate(
               asm_instructions.iterate_content(asm_.fn.instructions))) {
        std::println("#{}: {}", idx, print_instruction(instr, asm_operands));
      }
    }
  }

  if (error_manager.has_errors()) {
    error_manager.print_all_simple();
    return 4;
  }

  if (get_option(Option::StopAfterCodegen)) {
    return 0;
  }

  if (get_option(Option::EmitAsmFile)) {
    std::string output_file_asm = std::format("{}.S", output_file);
    std::ofstream output{output_file_asm, std::ios_base::out};
    output << to_gnu_as(asm_result) << "\n";

    if (output.bad()) {
      std::print("Writing to {} failed!", output_file_asm);
      return 5;
    }
  } else if (get_option(Option::PrintAsmFile)) {
    std::println("{}", to_gnu_as(asm_result));

    return 0;
  } else {
    return finish_compilation(asm_result);
  }

  return 0;
}

void Driver::set_option(Option option, bool value) {
  options.set(option, value);
}

bool Driver::get_option(Option option) {
  return options[option];
}

// More or less accurate...
bool which(const std::string& program) {
  const char* c_path = std::getenv("PATH");

  if (!c_path) {
    std::println("Could not determine value of $PATH, so could not find if {} "
                 "is installed",
                 program);
    return false;
  }

  // Disgusting C++ code tbh
  std::stringstream ss{c_path};
  std::string path{};

  while (std::getline(ss, path, ':')) {
    auto exec_path = std::filesystem::path{path} / program;

    if (std::filesystem::exists(exec_path)) {
      return true;
    }
  }

  return false;
}

template <std::same_as<std::string>... Ts>
bool run_with_arguments(const std::string& program, const Ts&... args) {
  int link_stdout[2] = {0, 0};
  int link_stderr[2] = {0, 0};

  if (pipe(link_stdout) == -1) {
    std::println("Creating a stdout pipe for {} failed: {}", program,
                 std::strerror(errno));
    return false;
  }

  if (pipe(link_stderr) == -1) {
    std::println("Creating a stderr pipe for {} failed: {}", program,
                 std::strerror(errno));
    return false;
  }

  pid_t pid = fork(); // Shenanigans

  if (pid == 0) {
    // Child
    dup2(link_stdout[1], STDOUT_FILENO);
    close(link_stdout[1]);
    close(link_stdout[0]);

    dup2(link_stderr[1], STDERR_FILENO);
    close(link_stderr[1]);
    close(link_stderr[0]);

    execlp(program.c_str(), program.c_str(), args.c_str()..., nullptr);

    std::println("Executing {} failed: {}", program, std::strerror(errno));

    exit(1); // execlp must have failed
  } else if (pid == -1) {
    std::println("Fork failed: {}", std::strerror(errno));
    return false;
  } else {
    // Original process

    close(link_stdout[1]);
    close(link_stderr[1]);

    auto print_from_link = [&program](int fd, bool is_stdout = true) {
      constexpr size_t buff_size = 1024;
      char buff[buff_size + 1] = {0};

      ssize_t bytes_read = read(fd, buff, buff_size);
      if (bytes_read == -1) {
        std::println("Error while reading {} from {}: {}",
                     is_stdout ? "stdout" : "stderr", program,
                     std::strerror(errno));
        return;
      } else if (bytes_read == 0) {
        return;
      } else {
        buff[bytes_read] = '\0';
      }

      std::println("{} {}:", program, is_stdout ? "out" : "err");
      std::print("{}", buff);
      while (true) {
        bytes_read = read(fd, buff, buff_size);
        if (bytes_read == -1) {
          std::println("Error while reading {} from {}: {}",
                       is_stdout ? "stdout" : "stderr", program,
                       std::strerror(errno));
          break;
        } else if (bytes_read > 0) {
          buff[bytes_read] = '\0';
          std::print("{}", buff);
        } else {
          std::println();
          break;
        }
      }
    };

    int status = 0;
    int res = waitpid(pid, &status, 0);

    if (res == -1) {
      std::println("Waiting for {} to finish failed: {}", program,
                   std::strerror(errno));
      print_from_link(link_stdout[0], true);
      print_from_link(link_stderr[0], false);
      return false;
    }

    if (!WIFEXITED(status)) {
      std::println("{} did not exit properly", program);
      print_from_link(link_stdout[0], true);
      print_from_link(link_stderr[0], false);
      return false;
    }

    if (WEXITSTATUS(status) != 0) {
      std::println("{} did not exit properly: Exit code was {}", program,
                   WEXITSTATUS(status));
      print_from_link(link_stdout[0], true);
      print_from_link(link_stderr[0], false);
      return false;
    }

    print_from_link(link_stdout[0], true);
    print_from_link(link_stderr[0], false);

    return true;
  }
}

int32_t Driver::finish_compilation(asm_parser::ParseResult asm_output) {
  using namespace std::string_literals;
  constexpr const char PROGRAM_AS[] = "riscv64-elf-as";
  constexpr const char PROGRAM_LD[] = "riscv64-elf-ld";
  constexpr const char PROGRAM_OBJCOPY[] = "riscv64-elf-objcopy";

  std::filesystem::path output_path{output_file};

  std::string tmp_name = output_path.parent_path() /
                         std::format(".0.{}", output_path.filename().string());

  /* Output to temporary file */
  std::string asm_file = std::format("{}.S", tmp_name);
  std::ofstream asm_out_file{asm_file, std::ios_base::out};
  asm_out_file << to_gnu_as(asm_output);
  asm_out_file.close();

  /* Verify programs are installed */
  if (!which(PROGRAM_AS)) {
    std::println("Could not find {}", PROGRAM_AS);
    return 6;
  }
  if (!which(PROGRAM_LD)) {
    std::println("Could not find {}", PROGRAM_LD);
    return 6;
  }
  if (!which(PROGRAM_OBJCOPY)) {
    std::println("Could not find {}", PROGRAM_OBJCOPY);
    return 6;
  }

  /* Assemble */
  std::string obj_file = std::format("{}.o", tmp_name);
  if (!run_with_arguments(PROGRAM_AS, asm_file, "-o"s, obj_file)) {
    std::println("Failed to assemble {}", asm_file);
    return 7;
  }

  /* Link */
  std::string elf_file = std::format("{}.elf", tmp_name);
  if (!run_with_arguments(PROGRAM_LD, "-melf64lriscv"s, "-nostdlib"s,
                          "-Ttext=0x80000000"s, obj_file, "-o"s, elf_file)) {
    std::println("Failed to link {}", obj_file);
    return 7;
  }

  /* Objcopy */
  std::string binary_file = std::format("{}.bin", output_file);
  if (!run_with_arguments(PROGRAM_OBJCOPY, elf_file, "-O"s, "binary"s,
                          binary_file)) {
    std::println("Failed to objcopy {}", elf_file);
    return 7;
  }

  return 0;
}

lexer::Lexer Driver::get_lexer(const std::string& filename,
                               error::ErrorManager& error_manager) {
  std::ifstream file{filename};
  std::string content{std::istreambuf_iterator<char>{file},
                      std::istreambuf_iterator<char>{}};

  auto source = std::make_shared<source::Source>(filename, content);

  lexer::Lexer lex{source, error_manager};

  return lex;
}

void Driver::print_help() {
  // TODO Move to another file
  // clang-format off
constexpr char help_text[] =
R"(nn : Compiler for the nn language

Compiles .nn files into RISC-V binary blobs, or RISC-V assembly files.
Currently requires riscv64-elf-{as, ld, objcopy} to be installed on the system.

USAGE: nn <FILE> [--help] [--lex] [--parse [--dot]] [--tac] [--codegen] [--asm] [--silent] [-o|--output <PATH>] [-S]

OPTIONAL PARAMETERS
  -o, --output: Path to output file, without extension
  -S: Emit an assembly file instead of a binary
  
  --help: Show this help
  --lex: Only go up to lexing, then print the tokens
  --parse: Only go up to parsing, then print the asts
    --dot: Show parse output as a dot file instead
  --tac: Only go up to parsing, then print the asts
  --codegen: Only go up to codegen, then print the program
  --asm: Instead of linking a binary, print the asm to stdout
  --silent: Do not output to stdout after finishing phases
)";
  // clang-format on

  std::println("{}", help_text);
}