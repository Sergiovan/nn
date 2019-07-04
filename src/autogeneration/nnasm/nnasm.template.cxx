#include "backend/nnasm.h"
//$move src/backend

using namespace nnasm;

// format::operand::operand(u16 raw) : raw(raw) { }

format::instruction::instruction(u16 op1, u16 op2, u16 op3) { 
    instruction::op[0].raw = op1;
    instruction::op[1].raw = op2;
    instruction::op[2].raw = op3;
    _empty = 0;
}

const std::array<std::vector<std::pair<format::instruction, opcode_internal>>, (u64) opcode::_LAST> format::get_formats() {
    using namespace format;
    
    std::array<std::vector<std::pair<instruction, opcode_internal>>, (u64) opcode::_LAST> ret{{
        //$replace format_list
    }};
    
    return ret;
}


std::ostream& operator<<(std::ostream& os, opcode code) {
    return os << op_to_name.at(code);
}
