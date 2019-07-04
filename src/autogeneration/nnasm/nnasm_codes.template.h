#pragma once
//$move src/backend
#include "common/convenience.h"

namespace nnasm {
    enum class opcode : u8 {
        //$replace op_enum
    };
    
    enum class opcode_internal : u16 {
        //$replace op_enum_internal
    };
    
    const dict<opcode> name_to_op {
        //$replace op_names
    };
    
    const std::map<opcode, std::string> op_to_name {
        //$replace op_names_reverse
    };
    
    const std::map<opcode_internal, std::string> op_to_name_internal {
        //$replace op_names_internal
    };
}