#pragma once

#include <variant>
#include "convenience.h"

struct value {
    u64 value;
    u64 type = 0;
    u8 flags = 0;
    u8 bitsize = 0;
    bool empty = false;
};
