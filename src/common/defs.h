#pragma once

#include <stdint.h>
#include <unordered_map>

#ifdef DEBUG
#include <iostream>

constexpr bool debug = true;

inline void _assert(bool condition, const char* msg, const char* func) {
    if (!condition) {
        std::cerr << msg << func << "\n";
        std::abort();
    }
}

#define __STRINGIFY(x) #x
#define __TOSTRING(x) __STRINGIFY(x)
#define ASSERT(x, msg) \
do { \
    _assert(!!(x), "Assertion \"" #x "\" failed: " #msg " at " __FILE__ ":" __TOSTRING(__LINE__) " in ", __PRETTY_FUNCTION__); \
} while (false)


#else

constexpr bool debug = false;

#define ASSERT(x, msg) 

#endif

template <typename K, typename V>
using dict = std::unordered_map<K, V>;

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using s8  = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

using f32 = float;
using f64 = double;

using c32 = u32;
