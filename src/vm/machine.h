#pragma once

#include <chrono>
#include "common/asm.h"
#include "common/convenience.h"

class parser;

struct vmregister {
    union {
        u64 byte : 8;
        i64 sbyte : 8;
        u64 word : 16;
        i64 sword : 16;
        u64 dword : 32;
        i64 sdword : 32;
        u64 qword{0};
        i64 sqword;
        float fl;
        double db;
    };
};

struct vmflagsregister {
    u64 zero : 1;
    u64 check : 1;
    u64 reserved : 62;
};

class virtualmachine {
public:
    virtualmachine(parser& p);
    ~virtualmachine();
    
    virtualmachine(const virtualmachine&) = delete;
    virtualmachine(virtualmachine&&) = delete;
    virtualmachine& operator=(const virtualmachine&) = delete;
    virtualmachine& operator=(virtualmachine&&) = delete;
    
    void run();
    void step();
    void load_instruction();
    void execute_instruction();
    void stop();
    
    void load(u8* program, u64 size);
    void allocate(u64 amount);
    void resize(u64 amount);
    void clear();
    
    std::string print_info();
    std::string print_register(u8 reg);
private:
    vmregister registers[16]{};
    vmregister& ra = registers[0];
    vmregister& rb = registers[1];
    vmregister& rc = registers[2];
    vmregister& rd = registers[3];
    vmregister& re = registers[4];
    vmregister& rf = registers[5];
    vmregister& rg = registers[6];
    vmregister& rh = registers[7];
    vmregister& ri = registers[8];
    vmregister& rj = registers[9];
    vmregister& rk = registers[10];
    vmregister& rl = registers[11];
    vmregister& rm = registers[12];
    vmregister& pc = registers[13];
    vmflagsregister& sf = *reinterpret_cast<vmflagsregister*>(&registers[14]);
    vmregister& sp = registers[15];
    
    nnasm::instruction loaded{};
    
    u8* memory{nullptr}; // Owned
    u8* code{nullptr};
    u8* data{nullptr};
    u8* heap{nullptr};
    u8* end{nullptr};
    
    u64 allocated{0};
    u64 read_only_end{0};
    u64 stack_size{0};
    
    decltype(std::chrono::high_resolution_clock::now()) start_time{};
    decltype(std::chrono::high_resolution_clock::now()) end_time{};
    u64 steps{};
    
    parser& p;

    bool running{false};
    bool stopped{false};    
};
