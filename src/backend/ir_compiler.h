#pragma once

#include <sstream>
#include "common/ir.h"

struct register_data {
    u64 last_use{0};
    bool memory{false};
    
};

class ir_compiler {
public:
    ir_compiler();
    
    void compile(ir_triple* from, const std::string& file, bool print = false);
private:
    std::stringstream ss{};
    
    u8 get_register();
    
};
