#include "vm/machine.h"

#include <cstring>
#include "frontend/parser.h"

using vm = virtualmachine;

vm::virtualmachine(parser& p) : p(p) {
    
}

vm::~virtualmachine() {
    clear();
}

void vm::run() {
    stopped = false;
    start_time = std::chrono::high_resolution_clock::now();
    while (running && !stopped) {
        step();
    }
    if (!running) {
        end_time = std::chrono::high_resolution_clock::now();
    }
}

void vm::step() {
    execute_instruction();
    ++steps;
    load_instruction();
}

void vm::load_instruction() {
    
}

void vm::execute_instruction() {
    
}

void vm::stop() {
    stopped = true;
}

void vm::load(u8* program, u64 size) {
    clear();
    u64* hdr = reinterpret_cast<u64*>(program); // TODO Proper header struct please
    u64 code_start = hdr[1];
    u64 data_start = hdr[2];
    u64 file_size  = hdr[3];
    u64 init_alloc = hdr[4]; 
    
    allocate(file_size + init_alloc);
    
    code = memory + code_start;
    read_only_end = data_start;
    data = memory + data_start;
    heap = memory + file_size;
    stack_size = std::min(init_alloc >> 1, (u64) 4 << 16); // 4 MB
    
    std::memcpy(memory, program, file_size);
    pc.qword = code_start;
    running = true;
    load_instruction();
}

void vm::allocate(u64 amount) {
    if (memory) {
        u64 code_diff = code - memory;
        u64 data_diff = data - memory;
        u64 heap_diff = heap - memory;
        u8* buff = new u8[allocated + amount + 8];
        std::memcpy(buff, memory, allocated - stack_size);
        std::memcpy(buff + allocated + amount - stack_size, end - stack_size, stack_size);
        delete [] memory;
        memory = buff;
        allocated += amount;
        code = memory + code_diff;
        data = memory + data_diff;
        heap = memory + heap_diff;
        stack_size = std::min(allocated >> 1, (u64) 4 << 16); // 4 MB
        end  = memory + allocated + 1;
    } else {
        memory = new u8[amount + 8]; // TODO assume non-infinite amount of memory
        allocated = amount;
        end = memory + allocated + 1;
    }
}

void vm::resize(u64 amount) {
    if (memory) {
        u64 code_diff = code - memory;
        u64 data_diff = data - memory;
        u64 heap_diff = heap - memory;
        u8* buff = new u8[amount + 8];
        std::memcpy(buff, memory, allocated - stack_size);
        std::memcpy(buff + amount - stack_size, end - stack_size, stack_size);
        delete [] memory;
        memory = buff;
        allocated = amount;
        code = memory + code_diff;
        data = memory + data_diff;
        heap = memory + heap_diff;
        stack_size = std::min(allocated >> 1, (u64) 4 << 16); // 4 MB TODO Fix stacks and heaps growth
        end  = memory + allocated + 1;
    } else {
        memory = new u8[amount + 8];
        allocated = amount;
        end = memory + allocated + 1;
    }
}

void vm::clear() {
    if (memory) {
        delete [] memory;
        memory = nullptr;
        code = nullptr;
        data = nullptr;
        heap = nullptr;
        end = nullptr;
    }
    
    allocated = 0;
    read_only_end = 0;
    stack_size = 0;
    
    std::memset(registers, 0, sizeof(vmregister) * 16);
    std::memset(&loaded, 0, sizeof(nnasm::instruction));
    
    running = false;
    stopped = false;
}

std::string vm::print_info() {
    using namespace std::chrono;
    std::stringstream ss{};
    if (!running) {
        ss << "Program done\n";
        duration program = end_time - start_time;
        ss << "Program took " << duration_cast<microseconds>(program).count() << "us\n";
        ss << "At " << ((double) steps / duration_cast<seconds>(program).count()) << " instructions/sec\n";
    } else {
        if (stopped) {
            ss << "Program stopped\n";
        } else {
            ss << "Program running\n";
        }
    }
    ss << "Allocated: 0x" << std::hex << allocated << "\n";
    ss << " Read only memory: 0x" << read_only_end << "\n";
    ss << " Static memory: 0x" << (heap - data) << "\n";
    ss << " Heap size: 0x" << (((end - 1) - heap) - stack_size) << "\n";
    ss << " Stack size: 0x" << stack_size << "\n";
    ss << "\nRegisters:\n";
    for (u8 i = 0; i < 16; ++i) {
        ss << print_register(i) << "\n";
    }
    return ss.str();
}

std::string vm::print_register(u8 reg) {
    using namespace std::string_literals;
    std::stringstream ss{};
    ss << std::hex;
    static const std::string endings[] = {""s, "s"s, "8"s, "8s"s, "16"s, "16s"s, "32"s, "32s"s, "f"s, "d"s};
    if (reg == 14) {
        ss << "sf: \n";
        ss << "Zero: " << sf.zero << "\n";
        ss << "Check: " << sf.check << "\n";
    } else {
        std::string name = reg < 13 ? ("r"s + (char)('a' + reg)) : (reg == 13 ? "pc"s : "sp"s);
        for (u8 i = 0; i < sizeof(endings) / sizeof(std::string); ++i) {
            ss << name << endings[i] << ": ";
            switch (i) {
                case 0:
                    ss << registers[reg].qword << "\n";
                    break;
                case 1:
                    ss << registers[reg].sqword << "\n";
                    break;
                case 2:
                    ss << registers[reg].byte << "\n";
                    break;
                case 3:
                    ss << registers[reg].sbyte << "\n";
                    break;
                case 4: 
                    ss << registers[reg].word << "\n";
                    break;
                case 5:
                    ss << registers[reg].sword << "\n";
                    break;
                case 6:
                    ss << registers[reg].dword << "\n";
                    break;
                case 7:
                    ss << registers[reg].sdword << "\n";
                    break;
                case 8:
                    ss << registers[reg].fl << "\n";
                    break;
                case 9:
                    ss << registers[reg].db << "\n";
                    break;
            }
        }
    }
    return ss.str();
}
