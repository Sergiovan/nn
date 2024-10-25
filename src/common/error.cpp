#include <iostream>

#include <sys/ioctl.h>
#include <unistd.h>

#include "common/error.hpp"

#include "common/ast.hpp"
#include "common/token.hpp"

#include "util/string.hpp"

using namespace error;

using ast::Ast;
using token::Token;

ErrorManager::ErrorManager() : errors{} {}

void ErrorManager::add_error(Error&& e) {
  errors.push_back(std::make_unique<Error>(std::move(e)));
}

bool ErrorManager::has_errors() const {
  return !errors.empty();
}

std::string ErrorManager::simple(const Error& e) const {
  std::stringstream ss{};
  auto& source = e.get_source();
  std::print(ss, "{}Error{} @ {}: {}\n", util::BOLD_RED, util::RESET_COLOR,
             source.get_file_location(), e.get_message());

  std::string line = source.get_full_line();
  std::string_view view{line};
  if (!line.empty()) {
    std::print(ss, "-> {}{}{}{}{}\n", view.substr(0, source.column),
               util::BOLD_WHITE, view.substr(source.column, source.length),
               util::RESET_COLOR, view.substr(source.column + source.length));
  }

  const Error* cause = e.get_cause();
  while (cause) {
    const Error& c = *cause;
    auto& source = c.get_source();
    std::print(ss, "Caused by {}: {}\n", source.get_file_location(),
               c.get_message());
    std::string line = source.get();
    if (!line.empty()) {
      std::print(ss, "-> {}\n", line);
    }

    cause = c.get_cause();
  }

  return ss.str();
}

std::string ErrorManager::fancy(u16 width, const Error& e) const {
  return "";
}

void ErrorManager::print_simple(const Error& e) const {
  print_simple(std::cout, e);
}

void ErrorManager::print_simple(std::ostream& os, const Error& e) const {
  os << simple(e);
}

void ErrorManager::print_fancy(const Error& e) const {
  print_fancy(std::cout, get_terminal_width(), e);
}

void ErrorManager::print_fancy(std::ostream& os, u16 width,
                               const Error& e) const {
  os << fancy(width, e);
}

void ErrorManager::print_all_simple() const {
  print_all_simple(std::cout);
}

void ErrorManager::print_all_simple(std::ostream& os) const {
  for (const auto& e : errors) {
    print_simple(std::cout, *e);
  }
}

void ErrorManager::print_all_fancy() const {
  print_all_fancy(std::cout, get_terminal_width());
}

void ErrorManager::print_all_fancy(std::ostream& os, u16 width) const {
  for (const auto& e : errors) {
    print_fancy(std::cout, width, *e);
  }
}

u16 ErrorManager::get_terminal_width() {
  // From https://stackoverflow.com/a/23370070
  winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

  return w.ws_col;
}

Error::Error() : source{source::nullloc}, error_msg{} {}

Error::Error(const std::string& msg)
    : source{source::nullloc}, error_msg{msg} {}

Error::Error(const token::Token& token, const std::string& msg)
    : source{token.loc}, error_msg{msg} {}

Error::Error(const ast::Ast& ast, const std::string& msg)
    : source{ast.source_location()}, error_msg{msg} {}

void Error::link_to(Error& root) {
  linked = &root;
}

const source::SourceLocation& Error::get_source() const {
  return source;
}

std::string_view Error::get_message() const {
  return error_msg;
}

const Error* Error::get_cause() const {
  return linked;
}