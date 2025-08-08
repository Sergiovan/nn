#pragma once

#include <array>
#include <type_traits>
#include <utility>

#include "util/enum.hpp"
#include "util/types.hpp"

namespace _enum_map_detail {

/* Helper function to determine the amount of bits required to store 
   a number in a bitmap*/
template <constrained_enum T>
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

/* Stores enum values as a bitmap */
template <constrained_enum T>
class EnumBitMap {
public:
  EnumBitMap() {};

  void set(T t, bool val) {
    U value = static_cast<U>(t);
    u64 cell = value / bits;
    u64 bit = value & (bits - 1);

    if (val) {
      data[cell] = static_cast<B>(data[cell] | (1u << bit));
    } else {
      data[cell] = data[cell] & ~static_cast<B>(1u << bit);
    }
  }

  bool get(T t) const {
    U value = static_cast<U>(t);
    u64 cell = value / bits;
    u64 bit = value & (bits - 1);

    B cell_data = data[cell];
    return (cell_data >> bit) & 1;
  }

  bool operator[](T t) {
    return get(t);
  }

private:
  using U = std::underlying_type_t<T>;
  static constexpr auto bits = _enum_map_detail::bits<T>();
  using B = std::remove_const_t<std::make_unsigned_t<decltype(bits)>>;

  std::array<B, ((to_underlying(T::LAST) - 1) / bits) + 1> data{{0}};
};
