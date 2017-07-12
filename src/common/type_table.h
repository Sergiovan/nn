#pragma once

#include <vector>

#include "convenience.h"
#include "symbol_table.h"
#include "type.h"
#include "value.h"


class type_table {
public:
    type_table();
    
    uid add_type(type t);
private:
    std::vector<type> types{};
};
