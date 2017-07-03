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

symbol_table& globals::get_symbol_table() {
    return st;
}

type_table& globals::get_type_table() {
    return tt;
}