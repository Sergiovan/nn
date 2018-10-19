#pragma once

#include <cstdint>
#include <string>
#include <map>

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using utfchar = u32;

template <typename T>
using dict = std::map<std::string, T>;

using type_id = u64;
using type_flags = u8;

constexpr bool __debug =
#ifdef DEBUG
                            true;
#else
                            false;
#endif
