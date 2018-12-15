#pragma once

#include <fstream>
#include <vector>
#include "common/asm.h"
#include "common/convenience.h"

/* Please note: This entire class is fucking garbage, a quick hack. 
 * Must be revisited and rewritten entirely. */

class asm_compiler {
public:
    asm_compiler(const std::string& filename);
    
    // Rule of 5 not needed for now, nothing fancy
    
    void compile();
    
    u8* get(); // Still owned by asm_compiler, do not delete
    u8* move(); // Delete when done plox
    void store_to_file(const std::string& filename);
    
private:
    std::ifstream file;
    bool ready{false};
    std::vector<u8> compiled{};
};
