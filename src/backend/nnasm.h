#pragma once

#include <type_traits>
#include <map>
#include <vector>
#include <iostream>
#include "common/convenience.h"
#include "backend/nnasm_codes.generated.h"

struct nnexe_header {
    char magic[4] = {'N', 'N', 'E', 'P'};
    u32 version = 0;
    u64 code_start = 128;
    u64 data_start = 0;
    u64 size = 0;
    u64 initial = 4 << 20;
    u8  _empty[88] = { 0 };
};

namespace nnasm {
    enum class opertype : u8 {
        NONE = 0b00, 
        REG  = 0b01, 
        VAL  = 0b10, 
        MEM  = 0b11
    };
    
    enum class operlen : u8 {
         _8 = 0b00, 
        _16 = 0b01, 
        _32 = 0b10, 
        _64 = 0b11
    };
    
    enum class data_type {
        U8, U16, U32, U64,
        S8, S16, S32, S64,
        F32, F64, NONE
    };
    
    constexpr u8 data_type_size(data_type type) {
        switch (type) {
            case data_type::U8:  return 1;
            case data_type::U16: return 2;
            case data_type::U32: return 4;
            case data_type::U64: return 8;
            case data_type::S8:  return 1;
            case data_type::S16: return 2;
            case data_type::S32: return 4;
            case data_type::S64: return 8;
            case data_type::F32: return 4;
            case data_type::F64: return 8;
            case data_type::NONE: return 0;
            default: return 0;
        }
    }
    
    constexpr data_type type_of_size(u64 size) {
        switch (size) {
            default: [[fallthrough]];
            case 8: return data_type::U64;
            case 7: [[fallthrough]];
            case 6: [[fallthrough]];
            case 5: [[fallthrough]];
            case 4: return data_type::U32;
            case 3: [[fallthrough]];
            case 2: return data_type::U16;
            case 1: return data_type::U8;
            case 0: return data_type::NONE;
        }
    }
    
#pragma pack(push, 1)
    
    // [location +- offset]
    struct mem_hdr {
        bool reg : 1; // Location is register
        u8 off_type : 2; // Type of offset: 0 None, 1 Register, 2 Negative register, 3 Immediate
        u8 _empty : 5;
    };
    
#pragma pack(pop)
    
    namespace registers {
        constexpr u8 general_start = 0;
        constexpr u8 r0 = general_start + 0;
        constexpr u8 r1 = general_start + 1;
        constexpr u8 r2 = general_start + 2;
        constexpr u8 r3 = general_start + 3;
        constexpr u8 r4 = general_start + 4;
        constexpr u8 r5 = general_start + 5;
        constexpr u8 r6 = general_start + 6;
        constexpr u8 r7 = general_start + 7;
        constexpr u8 r8 = general_start + 8;
        constexpr u8 r9 = general_start + 9;
        constexpr u8 r10 = general_start + 10;
        constexpr u8 r11 = general_start + 11;
        constexpr u8 r12 = general_start + 12;
        constexpr u8 r13 = general_start + 13;
        constexpr u8 r14 = general_start + 14;
        constexpr u8 r15 = general_start + 15;
        constexpr u8 general_amount = 16;
        
        constexpr u8 floating_start = general_start + general_amount;
        constexpr u8 f0 = floating_start + 0;
        constexpr u8 f1 = floating_start + 1;
        constexpr u8 f2 = floating_start + 2;
        constexpr u8 f3 = floating_start + 3;
        constexpr u8 f4 = floating_start + 4;
        constexpr u8 f5 = floating_start + 5;
        constexpr u8 f6 = floating_start + 6;
        constexpr u8 f7 = floating_start + 7;
        constexpr u8 f8 = floating_start + 8;
        constexpr u8 f9 = floating_start + 9;
        constexpr u8 f10 = floating_start + 10;
        constexpr u8 f11 = floating_start + 11;
        constexpr u8 f12 = floating_start + 12;
        constexpr u8 f13 = floating_start + 13;
        constexpr u8 f14 = floating_start + 14;
        constexpr u8 f15 = floating_start + 15;
        constexpr u8 floating_amount = 16;
        
        constexpr u8 special_start = floating_start + floating_amount;
        constexpr u8 pc = special_start + 0;
        constexpr u8 sf = special_start + 1;
        constexpr u8 sp = special_start + 2;
        constexpr u8 fp = special_start + 3;
        constexpr u8 special_amount = 4;
        
        constexpr u8 total_amount = general_amount + floating_amount + special_amount;
    }
    
    namespace format {
        namespace raw {
            constexpr u16 _u8  = 1u << 0;
            constexpr u16 _u16 = 1u << 1;
            constexpr u16 _u32 = 1u << 2;
            constexpr u16 _u64 = 1u << 3;
            
            constexpr u16 _s8  = 1u << 4;
            constexpr u16 _s16 = 1u << 5;
            constexpr u16 _s32 = 1u << 6;
            constexpr u16 _s64 = 1u << 7;
            
            constexpr u16 _f32 = 1u << 8;
            constexpr u16 _f64 = 1u << 9;
            
            constexpr u16 imm = 1u << 10;
            constexpr u16 reg = 1u << 11;
            constexpr u16 mem = 1u << 12;
            
            constexpr u16 uint = _u8 | _u16 | _u32 | _u64;
            constexpr u16 sint = _s8 | _s16 | _s32 | _s64;
            constexpr u16 _int = sint | uint;
            constexpr u16 real = _f32 | _f64;
            
            constexpr u16 any_type = _int | real;
            constexpr u16 byte = _u8 | _s8;
            
            constexpr u16 any_reg = reg | any_type;
            constexpr u16 any_imm = imm | any_type;
            constexpr u16 any_mem = mem | any_type;
            
            constexpr u16 any_target = reg | imm | mem;
            constexpr u16 any = any_target | any_type;
            
            constexpr u16 mem_loc = any_target | uint;
            constexpr u16 any_uint = mem_loc;
            constexpr u16 any_sint = any_target | sint;
            constexpr u16 any_int  = any_target | _int;
            constexpr u16 any_float = any_target | _f32;
            constexpr u16 any_double = any_target | _f64;
            constexpr u16 any_real = any_target | real;
            constexpr u16 any_byte = any_target | byte;
        };
        
        struct operand {
            union {
                u16 raw;
                struct {
                    bool _u8  : 1;
                    bool _u16 : 1;
                    bool _u32 : 1;
                    bool _u64 : 1;
                    
                    bool _s8  : 1;
                    bool _s16 : 1;
                    bool _s32 : 1;
                    bool _s64 : 1;
                    
                    bool _f32 : 1;
                    bool _f64 : 1;
                    
                    bool imm : 1;
                    bool reg : 1;
                    bool mem : 1;
                };
            };
        };
        
        struct instruction {
            instruction(u16 op1 = 0, u16 op2 = 0, u16 op3 = 0);
            
            union {
                struct {
                    operand op[3];
                    u16 _empty;
                };
                u64 raw;
            };
        };
        
        const std::array<std::vector<std::pair<instruction, opcode_internal>>, (u64) opcode::_LAST> get_formats();
    }
    
}

std::ostream& operator<<(std::ostream& os, nnasm::opcode code);
