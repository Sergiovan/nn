#pragma once

#include "common/ast.hpp"
#include "common/source.hpp"

namespace ast {
class Ast;
}

namespace token {
struct Token;
}

namespace error {

class Error;

/** Handles all compilation errors, including storing them and showing them. 
    Errors are formatted in two main ways: Simple and Fancy. 
    
    - Simple formatting has color but otherwise takes little effort to present
      errors in a readable way. Eventually meant to go away
    - Fancy formatting makes sure to fit on the screen, point at relevant info, 
      and mark locations of interest clearly */
class ErrorManager {
public:
  ErrorManager();

  /**
   * @brief Adds an error to the current list of compilation errors
   * 
   * @param e Error to add
   */
  void add_error(Error&& e);

  /** Returns true if any compilation errors have been found, false otherwise */
  bool has_errors() const;

  /**
   * @brief Puts an error into simple format
   * 
   * @param e The error to format
   * @return std::string Formatted error
   */
  std::string simple(const Error& e) const;

  /**
   * @brief Puts an error into fancy format
   * 
   * @param width Maximum width for format string
   * @param e The error to format
   * @return std::string Formatted error
   */
  std::string fancy(u16 width, const Error& e) const;

  /**
   * @brief Formats an error into simple format and prints it to stdout
   * 
   * @param e The error to format
   */
  void print_simple(const Error& e) const;
  /**
   * @brief Formats an error into simple format and prints it
   * 
   * @param os Where to print the error string to
   * @param e The error to print
   */
  void print_simple(std::ostream& os, const Error& e) const;

  /**
   * @brief Formats an error into fancy format and prints it to stdout
   * 
   * @param e The error to print
   */
  void print_fancy(const Error& e) const;
  /**
   * @brief Formats an error into fancy format and prints it
   * 
   * @param os Where to print the error string to
   * @param width Maximum width of error
   * @param e The error to print
   */
  void print_fancy(std::ostream& os, u16 width, const Error& e) const;

  /** Prints all errors in simple format to stdout */
  void print_all_simple() const;
  /**
   * @brief Prints all errors in simple format
   * 
   * @param os Where to print the error strings
   */
  void print_all_simple(std::ostream& os) const;

  /** Prints all errors in fancy format */
  void print_all_fancy() const;
  /**
   * @brief Prints all errors in fancy format
   * 
   * @param os Where to print the error strings
   * @param width Maximum widht of errors
   */
  void print_all_fancy(std::ostream& os, u16 width) const;

private:
  /** Gets the maximum width of stdout */
  static u16 get_terminal_width();

  std::vector<std::unique_ptr<Error>> errors;
};

/** Represents a compilation error */
class Error {
public:
  /** Creates an error from a source location, with a string message */
  Error(const source::SourceLocation loc, const std::string& msg);
  /** Creates an empty error */
  Error();
  /** Creates an error from a string, without source location */
  Error(const std::string& msg);
  /** Creates an error from a token, with a string message */
  Error(const token::Token& token, const std::string& msg);
  /** Creates an error from an ast node, with a string message */
  Error(const ast::Ast& ast, const ast::AstContainer& container,
        const std::string& msg);

  /** Links this error cause to another error */
  void link_to(Error& root);

  /** Get the source location which causes this error */
  const source::SourceLocation& get_source() const;
  /** Get the error message for this error */
  std::string_view get_message() const;
  /** Get the error cause for this error, if any */
  const Error* get_cause() const;

private:
  /** Source location for this error */
  source::SourceLocation source;
  /** Error message */
  std::string error_msg;
  /** Error cause of this error, if any */
  const Error* linked{nullptr}; // Not owned
};

} // namespace error