#include "vm/machine.h"

#include <iomanip>

#include "backend/nnasm.h"
#include "common/utils.h"

virtualmachine::virtualmachine() {
    
}

virtualmachine::~virtualmachine() {
    if (memory) {
        delete [] memory;
    }
}

void virtualmachine::load(u8* program, u64 size) {
    clear();
    
    nnexe_header* hdr = reinterpret_cast<nnexe_header*>(program);
    // TODO Header check
    u64 code_start = hdr->code_start;
    u64 data_start = hdr->data_start;
    u64 file_size  = hdr->size;
    u64 initial_alloc = hdr->initial;
    
    allocate(file_size + initial_alloc);
    
    code = memory + code_start;
    read_only_end = data_start;
    data = memory + data_start;
    heap = memory + file_size;
    stack_size = std::min(initial_alloc >> 1, (u64) 1 << 23); // Half the space or 4MB
    
    std::memcpy(memory, program, size);
}

void virtualmachine::run() {
    started = true;
    paused = false;
    if (ended) {
        pc._u64 = code - memory; // Start of code
        sp._u64 = allocated;
        fp._u64 = allocated;
        ended = false;
        steps = 0;
        start_time = std::chrono::high_resolution_clock::now();
    }
    
    while (!ended && !to_pause) {
        step();
    }
    
    if (to_pause) {
        to_pause = false;
        paused = true;
    }
    if (ended) {
        started = false;
        end_time = std::chrono::high_resolution_clock::now();
    }
}

void virtualmachine::pause() {
    to_pause = true;
}

void virtualmachine::stop() {
    ended = true;
}

std::string virtualmachine::print_info() {
    using namespace std::chrono;
    std::stringstream ss{};
    if (ended) {
        ss << "Program done\n";
        auto time = end_time - start_time;
        ss << "Program took " << duration_cast<microseconds>(time).count() << "us\n";
        ss << "Executed " << steps << " instructions @ " << ((double) steps / (double) duration_cast<seconds>(time).count()) << " instructions/sec\n";
#if PROFILE
        ss << "Took " << loading << "ns loading instructions\n";
        ss << "Took " << executing << "ns executing instructions\n";
#endif
    } else if (paused) {
        ss << "Program paused\n";
    } else {
        ss << "Program running\n";
    }
    
    ss << std::hex << "Allocated 0x" << allocated << " bytes\n";
    ss << " Read only memory: 0x" << read_only_end << " bytes\n";
    ss << " Static memory: 0x" << (heap - data) << " bytes\n";
    ss << " Heap size: 0x" <<  (((end - 1) - heap) - stack_size) << " bytes\n";
    ss << " Stack size: 0x" << stack_size << " bytes\n";
    return ss.str();
}

std::string virtualmachine::print_registers() {
    std::stringstream ss{};
    static const char* general_names[general_register_amount] {
        "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8",
        "r9", "r10", "r11", "r12", "r13", "r14", "r15"
    };
    static const char* floating_names[floating_register_amount] {
        "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8",
        "f9", "f10", "f11", "f12", "f13", "f14", "f15"
    };
    static const char* special_names[special_register_amount] {
        "pc", "sf", "sp", "fp"
    };
    for (int i = 0; i < general_register_amount; ++i) {
        ss << general_names[i] << ": \n";
        ss << "   u: " << std::hex << general_registers[i]._u64 << "\n";
        ss << "   s: " << std::dec << general_registers[i]._s64 << "\n";
    }
    for (int i = 0; i < floating_register_amount; ++i) {
        ss << floating_names[i] << ": \n";
        ss << "   f: " << floating_registers[i]._f32 << "\n";
        ss << "   d: " << floating_registers[i]._f64 << "\n";
    }
    for (int i = 0; i < special_register_amount; ++i) {
        ss << special_names[i] << ": \n";
        ss << "   u: " << std::hex << special_registers[i]._u64 << "\n";
    }
    
    return ss.str();
}

void virtualmachine::allocate(u64 amount) {
    if (memory) {
        u64 code_start = code - memory;
        u64 data_start = read_only_end;
        u64 file_size = heap - memory;
        
        u8* buff = new u8[allocated + amount + 8]; // TODO Catch out_of_memory
        std::memcpy(buff, memory, allocated - stack_size);
        std::memcpy((buff + allocated + amount) - stack_size, memory + allocated - stack_size, stack_size);
        delete [] memory;
        memory = buff;
//         sp._u64 += amount;
        allocated += amount;
        
        code = memory + code_start;
        read_only_end = data_start;
        data = memory + data_start;
        heap = memory + file_size;
        stack_size = std::min((allocated - file_size) >> 1, (u64) 1 << 23); // Half the space or 4MB
        end = memory + allocated + 1;
    } else {
        memory = new u8[amount + 8]; // TODO Catch out_of_memory
        allocated = amount;
        end = memory + allocated + 1;
    }
}

void virtualmachine::resize(u64 amount) {
    if (memory) {        
        u64 code_start = code - memory;
        u64 data_start = read_only_end;
        u64 file_size = heap - memory;
        
        // TODO Check viability with heap size via allocator
        // TODO Check math? 
        
        u8* buff = new u8[amount + 8];
        std::memcpy(buff, memory, allocated - stack_size);
        std::memcpy((buff + amount) - stack_size, memory + allocated - stack_size, stack_size);
        delete [] memory;
        memory = buff;
//         sp._u64 += (amount - allocated);
        allocated = amount;
        
        code = memory + code_start;
        read_only_end = data_start;
        data = memory + data_start;
        heap = memory + file_size;
        stack_size = std::min((allocated - file_size) >> 1, (u64) 1 << 23); // Half the space or 4MB
        end = memory + allocated + 1;
    } else {
        memory = new u8[amount + 8]; // TODO Catch out_of_memory
        allocated = amount;
        end = memory + allocated + 1;
    }
}

void virtualmachine::clear() {
    if (memory) {
        delete [] memory;
    }
    
    memory = code = data = heap = end = nullptr;
    allocated = read_only_end = stack_size = 0;
    
    std::memset(general_registers,  0, sizeof general_registers);
    
    started = to_pause = paused = false;
    ended = true;
}

void virtualmachine::trap(s64 signal) {
    // TODO Do something
    if (signal > 0) {
        logger::error() << "Got signal " << signal << logger::nend;
    } else {
        logger::info() << "Got signal " << signal << logger::nend;
    }
}

