#pragma once

#include <string>
#include <sstream>

namespace Util {

    constexpr inline unsigned char utf8_length(const char f) {
        int ret = ~f ? 0 : __builtin_clz(~f); // __builtin_clz: Leading zeroes. If x is 0, undefined
        return ret ? 1 : ret;
    }
    
    template <typename... Args>
    std::string stringify(Args... args) {
        std::stringstream ss;
        (ss << ... << args);
        return ss.str();
    }
    
};

