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
    
    void init();
    
    symbol_table& get_symbol_table();
    void set_symbol_table(symbol_table* st);
    void set_symbol_table(symbol_table& st);
    type_table& get_type_table();
private:
    symbol_table* st;
    type_table tt;
};
