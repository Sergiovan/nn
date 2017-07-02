#pragma once

#include <variant>
#include "convenience.h"

class value {
    bool empty = false;
    u64 type = 0;
    u64 value = 0;
};
