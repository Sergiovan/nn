//$move src/vm
#include <chrono>
#include "vm/machine.h"
#include "backend/nnasm.h"

void virtualmachine::step() {
    using namespace nnasm;
    using namespace std::chrono;
    
    // BEGIN Load instruction
#if PROFILE
    auto begin_load = std::chrono::high_resolution_clock::now();
#endif
    using instr_type = u16;
    
    align<2>();
    instr_type instr = read_from_pc<instr_type>();
    
    // END Load instruction
#if PROFILE
    loading += duration_cast<nanoseconds>(std::chrono::high_resolution_clock::now() - begin_load).count();
    
    auto begin_execute = std::chrono::high_resolution_clock::now();
#endif
    // BEGIN Execute instruction
    switch ((opcode_internal) instr) {
        //$replace vm_code
        default: {
            trap(vmtraps::illegal_instruction);
            break;
        }
    }
    // END Execute instruction
#if PROFILE
    executing += duration_cast<nanoseconds>(std::chrono::high_resolution_clock::now() - begin_execute).count();
#endif
    ++steps;
}