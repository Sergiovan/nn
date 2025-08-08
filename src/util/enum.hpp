#pragma once

#include <type_traits>

template <typename T>
concept is_enum = std::is_enum_v<T>;

template <typename T>
concept constrained_enum = is_enum<T> && requires {
  { T::FIRST } -> std::same_as<T>;
  { T::LAST } -> std::same_as<T>;

  static_cast<std::underlying_type_t<T>>(T::LAST) >
      static_cast<std::underlying_type_t<T>>(T::FIRST);
};

template <is_enum T>
constexpr std::underlying_type_t<T> to_underlying(const T t) {
  return static_cast<std::underlying_type_t<T>>(t);
}