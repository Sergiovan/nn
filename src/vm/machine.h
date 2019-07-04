#pragma once

#include <chrono>
#include <cstring>
#include <type_traits>

#include "common/convenience.h"
#include "backend/nnasm.h"

#define PROFILE false

struct vmregister {
    union {
        u8 _u8;
        u16 _u16;
        u32 _u32;
        u64 _u64;
        s8 _s8;
        s16 _s16;
        s32 _s32;
        s64 _s64;
        f32 _f32;
        f64 _f64;
    };
};

namespace vmflags {
    constexpr u64 zero  = 1ull << 0;
    constexpr u64 check = 1ull << 1;
};

namespace vmtraps {
    constexpr s64 halt = -2;
    constexpr s64 break_trap = -1;
    constexpr s64 illegal_read = 1;
    constexpr s64 illegal_write = 2;
    constexpr s64 stack_overflow = 3;
    constexpr s64 stack_underflow = 4;
    constexpr s64 illegal_jump = 5;
    constexpr s64 illegal_btin = 6;
    constexpr s64 illegal_instruction = 7;
};

struct vmflagsregister {
    u64 zero : 1;
    u64 check : 1;
    u64 _empty : 62;
};

class vm_helper;

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
    vmregister general_registers[19 + 16]{};
    vmregister& pc = general_registers[16];
    vmregister& sfr = general_registers[17];
    vmflagsregister& sf = *reinterpret_cast<vmflagsregister*>(&general_registers[17]);
    vmregister& sp = general_registers[18];
    vmregister* floating_registers = &general_registers[19];
    
    void allocate(u64 amount);
    void resize(u64 amount);
    void clear();
    
    void trap(s64 signal);
    
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
    
    u64 steps{0};
    decltype(std::chrono::high_resolution_clock::now()) start_time{};
    decltype(std::chrono::high_resolution_clock::now()) end_time{};
    
    u64 loading{0};
    u64 executing{0};
    
    template <typename T>
    inline T read_from_memory() {
        using namespace nnasm; 
        u8* mem = nullptr;
        align<sizeof(mem_hdr)>();
        mem_hdr hdr = read_from_pc<mem_hdr>();
        u64 base = 0;
        if (hdr.reg) {
            base = general_registers[read_from_pc<u8>()]._u64;
        } else {
            align<8>();
            base = read_from_pc<u64>();
        }
        s64 offset = 0;
        switch (hdr.off_type) {
            case 1: offset =  general_registers[read_from_pc<u8>()]._s64; break;
            case 2: offset = -general_registers[read_from_pc<u8>()]._s64; break;
            case 3: align<8>(); offset = read_from_pc<s64>(); break;
            default: break;
        }
        mem = memory + (base + offset);
        if constexpr (std::is_same_v<T, u8>) {
            return check(mem) ? *mem : (trap(vmtraps::illegal_read), (T) 0);
        } else {
            align<sizeof(T)>();
            return check(mem) ? *reinterpret_cast<T*>(mem) : (trap(vmtraps::illegal_read), (T) 0);
        }
    }
    
    template <typename T>
    inline T read_from_pc() {
        T r = *reinterpret_cast<T*>(memory + pc._u64);
        pc._u64 += sizeof(T);
        return r;
    }
    
    template <u8 amount>
    inline void align() {
        constexpr std::array<u8, 9> shift_masks {{0, 0, 1, 0, 0b11, 0, 0, 0, 0b111}};
        constexpr u8 mask = shift_masks[amount];
        if constexpr (amount == 0 || amount == 1) {
            
        } else if constexpr ((amount & (amount - 1)) == 0) {
            const u8 modulo = pc._u64 & mask;
            pc._u64 += (amount - modulo) * (modulo != 0);
        } else {
            static_assert((amount & (amount - 1)) == 0, "Calling align() with non-power amount");
        }
    }
    
    inline bool check(u8* ptr) {
        return (ptr >= data) && (ptr < end);
    }
    
    inline bool check(u8* ptr, s64 offs) {
        return check(ptr) && (ptr + offs < end) && (ptr + offs >= data);
    }
    
    inline bool check(u8* ptr, u64 offs) {
        return check(ptr) && (ptr + offs < end) && (ptr + offs >= data);
    }
    
    inline bool check_jump(u8* ptr) {
        return (ptr < data) && (ptr >= memory);
    }
    
    friend class vmhelper;
};
