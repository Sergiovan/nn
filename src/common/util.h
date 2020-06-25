#pragma once

#include <sstream>
#include "common/defs.h"

template <typename U, typename V>
dict<V, U> swap_map(dict<U, V>& m) {
    dict<V, U> ret{};
    
    for (auto& [k, v] : m) {
        ret.insert({v, k});
    }
    
    return ret;
}

namespace ss {
    
class streamstring;
    
struct streamstring_end {};

template <typename T>
struct get_res {
    using res = streamstring&;
};

template <>
struct get_res<streamstring_end> {
    using res = std::string;
};

template <typename T>
using res = typename get_res<T>::res;

class streamstring {
public:
    template <typename T>
    res<T> operator<<(const T& t) {
        if constexpr (std::is_same_v<std::remove_cv_t<T>, streamstring_end>) {
            return sss.str();
        } else {
            sss << t;
            return *this;
        }
    }
private:
    std::stringstream sss{};
};

streamstring get();
streamstring_end end();

}

template<typename F>
struct generic_guard {
    generic_guard(F f) : f{f} {}
    ~generic_guard() {
        f();
    }
    
    F f;
};

template<typename T, typename F> 
void for_each(T& t, F&& f) {
    for(auto e : t) {
        f(e);
    }
}

u64 parse_hex(const char* c, u8 max_len = 0xFF, u8* len = nullptr);
u64 utf8_to_utf32(const char* c, u8* len = nullptr);
u64 read_utf8(const char* c, u8& bytes, u8* len = nullptr);

u64 align(u64 offset, u64 to);
