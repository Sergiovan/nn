#pragma once

#include "types.hpp"
#include <print>
#include <ranges>
#include <stacktrace>

inline void print_relevant_stacktrace(const std::stacktrace& trace) {
  std::string fmt;
  u32 padding = std::to_string(trace.size() - 1).length();
  for (auto [i, entry] : std::views::enumerate(trace)) {
    fmt = std::format("{}", entry);
    std::println("{:>{}}# {}", i, padding, fmt);
    if (fmt.starts_with("main ")) {
      break;
    }
  }
}

#undef assert

#define assert(EXPR)                                                           \
  do {                                                                         \
    if (!(EXPR)) {                                                             \
      assert_fail(#EXPR, std::stacktrace::current());                          \
    }                                                                          \
  } while (0)

[[noreturn]] inline void assert_fail(const std::string& expr,
                                     const std::stacktrace& trace) {
  std::println("Expression failed: {}", expr);
  print_relevant_stacktrace(trace);
  std::exit(1);
}

#define unreachable                                                            \
  do {                                                                         \
    unreachable_fail(std::stacktrace::current());                              \
  } while (0)

[[noreturn]] inline void unreachable_fail(const std::stacktrace& trace) {
  std::println("Unreachable line was reached");
  print_relevant_stacktrace(trace);
  std::exit(1);
}