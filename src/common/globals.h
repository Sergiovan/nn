#pragma once

#include "symbol_table.h"
#include "type_table.h"

class globals;

namespace Globals {
    static globals* g = nullptr;
    
    void set_globals(globals& g);
    globals& get_globals();
};

class globals {
public:
    globals();
    
    symbol_table& get_symbol_table();
    type_table& get_type_table();
private:
    symbol_table st;
    type_table tt;
};
