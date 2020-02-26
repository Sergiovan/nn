#pragma once

#include <map>

template <typename U, typename V>
std::map<V, U> swap_map(std::map<U, V>& m) {
    std::map<V, U> ret{};
    
    for (auto& [k, v] : m) {
        ret.insert({v, k});
    }
    
    return ret;
}
