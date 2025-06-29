#pragma once

#include "types.hpp"
#include <iomanip>
#include <iostream>
#include <print>
#include <ranges>
#include <stacktrace>

inline void print_relevant_stacktrace(const std::stacktrace& trace) {
  std::string fmt;
  u64 padding = std::to_string(trace.size() - 1).length();
  for (auto [i, entry] : std::views::enumerate(trace)) {
    fmt = std::format("{}", entry);
    std::cout << std::setw(static_cast<int>(padding)) << std::left << i << "# "
              << fmt;
    // Note: For some reason clang 20.1.6 does not compile the following line
    // so I have replaced it with old timey std::cout. In the future, fix this
    // issue
    // error: call to consteval function 'std::basic_format_string<char, long &,
    // unsigned long &, std::basic_string<char> &>::basic_format_string<char[11]>'
    // is not a constant expression
    // std::println("{:>{}}# {}", i, padding, fmt);
    if (fmt.starts_with("main ")) {
      break;
    }
  }
}

/* Asserts `EXPR` to be true, otherwise exits the program */
#define nn_assert(EXPR)                                                        \
  do {                                                                         \
    if (!(EXPR)) {                                                             \
      assert_fail(#EXPR, std::stacktrace::current());                          \
    }                                                                          \
  } while (0)

/* Called when an assertion fails. Prints the expression that 
   failed and a stack trace, then exits */
[[noreturn]] inline void assert_fail(const std::string& expr,
                                     const std::stacktrace& trace) {
  std::println("Expression failed: {}", expr);
  print_relevant_stacktrace(trace);
  std::exit(1);
}

/* Terminates the program if this macro is executed */
#define unreachable                                                            \
  do {                                                                         \
    unreachable_fail(std::stacktrace::current());                              \
  } while (0)

/* Called when an unreachable line is executed. Prints the stack trace and
   then exits */
[[noreturn]] inline void unreachable_fail(const std::stacktrace& trace) {
  std::println("Unreachable line was reached");
  print_relevant_stacktrace(trace);
  std::exit(1);
}