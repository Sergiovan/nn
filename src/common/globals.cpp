#include "globals.h"

namespace Globals {
    void set_globals(globals& g) {
        *Globals::g = g;
    }
    
    globals& get_globals() {
        return *g;
    }
};

globals::globals() : st(nullptr), tt() { }

void globals::init() {
    if(!st) {
        st = new symbol_table;
    }
}

symbol_table& globals::get_symbol_table() {
    return *st;
}

void globals::set_symbol_table(symbol_table* st) {
    globals::st = st;
}

void globals::set_symbol_table(symbol_table &st) {
    globals::st = &st;
}

type_table& globals::get_type_table() {
    return tt;
}
