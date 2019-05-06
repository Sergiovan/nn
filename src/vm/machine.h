#pragma once

#include <cstring>

#include "common/convenience.h"

struct vmregister {
    union {
        u64 u;
        i64 s;
        float f;
        double d;
    };
};

namespace vmflags {
    constexpr u64 zero  = 1ull << 0;
    constexpr u64 check = 1ull << 1;
};

namespace vmtraps {
    constexpr i64 halt = -2;
    constexpr i64 break_trap = -1;
    constexpr i64 illegal_read = 1;
    constexpr i64 illegal_write = 2;
    constexpr i64 stack_overflow = 3;
    constexpr i64 stack_underflow = 4;
    constexpr i64 illegal_jump = 5;
    constexpr i64 illegal_btin = 6;
    constexpr i64 illegal_instruction = 7;
};

struct vmflagsregister {
    u64 zero : 1;
    u64 check : 1;
    u64 _empty : 62;
};

class virtualmachine {
public:
    virtualmachine();
    ~virtualmachine();
    
    virtualmachine(const virtualmachine&) = delete;
    virtualmachine(virtualmachine&&) = delete;
    virtualmachine& operator=(const virtualmachine&) = delete;
    virtualmachine& operator=(virtualmachine&&) = delete;
    
    void load(u8* program, u64 size);
    
    void run();
    void step();
    void pause();
    void stop();
    
    std::string print_info();
    std::string print_registers();
private:
    vmregister general_registers[19]{};
    vmregister& pc = general_registers[16];
    vmregister& sfr = general_registers[17];
    vmflagsregister& sf = *reinterpret_cast<vmflagsregister*>(&general_registers[17]);
    vmregister& sp = general_registers[18];
    vmregister floating_registers[16]{};
    
    void allocate(u64 amount);
    void resize(u64 amount);
    void clear();
    
    void trap(i64 signal);
    
    u8* memory{nullptr}; // Owned
    u8* code{nullptr}; 
    u8* data{nullptr};
    u8* heap{nullptr};
    u8* end{nullptr};
    
    u64 allocated{0};
    u64 read_only_end{0};
    u64 stack_size{0};
    
    bool started{false};
    bool to_pause{false};
    bool paused{false};
    bool ended{true};
    
    template <typename T>
    T read_from_pc() {
        T r;
        std::memcpy(&r, memory + pc.u, sizeof(T));
        pc.u += sizeof(T);
        return r;
    }
    
    template <typename T>
    T read(u64 pos) {
        if (pos + sizeof(T) > allocated) {
            trap(vmtraps::illegal_read);
            return T(0);
        }
        T r;
        std::memcpy(&r, memory + pos, sizeof(T));
        return r;
    }
    
    template <typename T>
    void write(T t, u64 pos) {
        if (pos + sizeof(T) > allocated) {
            trap(vmtraps::illegal_write);
            return;
        }
        if (pos < read_only_end) {
            trap(vmtraps::illegal_write);
            return;
        }
        std::memcpy(memory + pos, &t, sizeof(T));
    }
    
    template <typename T>
    void push(T t) {
        if (sp.u - sizeof(T) > allocated - stack_size) {
            trap(vmtraps::stack_overflow);
            return;
        }
        sp.u -= sizeof(T);
        std::memcpy(memory + sp.u, &t, sizeof(T));
    }
    
    template <typename T>
    T pop() {
        if (sp.u + sizeof(T) > allocated) {
            trap(vmtraps::stack_underflow);
            return T(0);
        }
        T r;
        std::memcpy(&r, memory + sp.u, sizeof(T));
        sp.u += sizeof(T);
        return r;
    }
};
