#pragma once

#include "common/defs.h"

template <typename U, typename V>
dict<V, U> swap_map(dict<U, V>& m) {
    dict<V, U> ret{};
    
    for (auto& [k, v] : m) {
        ret.insert({v, k});
    }
    
    return ret;
}
