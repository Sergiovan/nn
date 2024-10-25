#pragma once

#include "common/source.hpp"

namespace ast {
struct Ast;
}

namespace token {
struct Token;
}

namespace error {

class Error;

class ErrorManager {
public:
  ErrorManager();

  void add_error(Error&& e);
  bool has_errors() const;

  std::string simple(const Error& e) const;
  std::string fancy(u16 width, const Error& e) const;

  void print_simple(const Error& e) const;
  void print_simple(std::ostream& os, const Error& e) const;

  void print_fancy(const Error& e) const;
  void print_fancy(std::ostream& os, u16 width, const Error& e) const;

  void print_all_simple() const;
  void print_all_simple(std::ostream& os) const;

  void print_all_fancy() const;
  void print_all_fancy(std::ostream& os, u16 width) const;

private:
  static u16 get_terminal_width();

  std::vector<std::unique_ptr<Error>> errors;
};

class Error {
public:
  Error();
  Error(const std::string& msg);
  Error(const token::Token& token, const std::string& msg);
  Error(const ast::Ast& ast, const std::string& msg);

  void link_to(Error& root);

  const source::SourceLocation& get_source() const;
  std::string_view get_message() const;
  const Error* get_cause() const;

private:
  source::SourceLocation source;
  std::string error_msg;
  const Error* linked{nullptr}; // Not owned
};

} // namespace error