#pragma once

#include <print>

template <typename T> struct std::formatter<std::vector<T>> {
  constexpr auto parse(std::format_parse_context& ctx) {
    return ctx.begin();
  }

  auto format(const std::vector<T>& vec, std::format_context& ctx) const {
    std::format_to(ctx.out(), "[");
    for (auto& elem : vec) {
      std::format_to(ctx.out(), "{}, ", elem);
    }
    return std::format_to(ctx.out(), "]");
  }
};