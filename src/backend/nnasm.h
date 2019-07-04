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
    
#pragma pack(push, 1)
    
    // [location +- offset]
    struct mem_hdr {
        bool reg : 1; // Location is register
        u8 off_type : 2; // Type of offset: 0 None, 1 Register, 2 Negative register, 3 Immediate
        u8 _empty : 5;
    };
    
#pragma pack(pop)
    
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
