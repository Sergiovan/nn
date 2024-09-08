#pragma once

#include <array>
#include <type_traits>

#include "types.hpp"

template <typename T>
concept BoundedEnum = std::is_scoped_enum_v<T> && requires {
  { T::LAST } -> std::same_as<T>;
};

namespace _enum_map_detail {

template <BoundedEnum T>
consteval auto bits() {
  using U = std::underlying_type_t<T>;
  constexpr U last_value = static_cast<U>(T::LAST);

  if constexpr (last_value < 8) {
    return static_cast<u8>(8);
  } else if constexpr (last_value < 16) {
    return static_cast<u16>(16);
  } else if constexpr (last_value < 32) {
    return static_cast<u32>(32);
  } else {
    return static_cast<u64>(64);
  }
}

} // namespace _enum_map_detail

template <BoundedEnum T>
class EnumBitMap {
public:
  EnumBitMap() {};

  void set(T t, bool val) {
    U value = static_cast<U>(t);
    u16 cell = value / bits;
    u16 bit = value & (bits - 1);

    if (val) {
      data[cell] = data[cell] | (1 << bit);
    } else {
      data[cell] = data[cell] & ~static_cast<B>(1 << bit);
    }
  }

  bool get(T t) const {
    U value = static_cast<U>(t);
    u16 cell = value / bits;
    u16 bit = value & (bits - 1);

    B cell_data = data[cell];
    return (cell_data >> bit) & 1;
  }

  bool operator[](T t) {
    return get(t);
  }

private:
  using U = std::underlying_type_t<T>;
  static constexpr auto bits = _enum_map_detail::bits<T>();
  using B = std::remove_const_t<decltype(bits)>;

  std::array<B, ((static_cast<U>(T::LAST) - 1) / bits) + 1> data{{0}};
};
