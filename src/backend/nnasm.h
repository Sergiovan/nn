#pragma once

#include <type_traits>
#include <map>
#include <vector>
#include <iostream>
#include "common/convenience.h"

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
    enum class opcode : u8 {
        NOP, LOAD, STOR, MOV, CPY, ZRO, SET, BRK, HLT,
        CZRO, CNZR, CEQ, CNEQ, CBS, CBNS, 
        CLT, SCLT, FCLT, DCLT, CLE, SCLE, FCLE, DCLE, 
        CGT, SCGT, FCGT, DCGT, CGE, SCGE, FCGE, DCGE, 
        JMP, JMPR, SJMPR, JCH, JNCH, PUSH, POP, BTIN, CALL, RET,
        CSTU, CSTF, CSTD, CUTS, CUTF, CUTD, CFTS, CFTU, CFTD, CDTS, CDTU, CDTF, 
        ADD, SADD, FADD, DADD, INC, SINC, SUB, SSUB, FSUB, DSUB, DEC, SDEC,
        MUL, SMUL, FMUL, DMUL, DIV, SDIV, FDIV, DDIV, MOD, SMOD,
        SABS, FABS, DABS, SNEG, FNEG, DNEG, 
        SHR, SSHR, SHL, SSHL, RTR, RTL, 
        AND, OR, XOR, NOT,
        
        VAL, DB, DBS, LBL,
        INV = 0xFF
    };
    
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
    
    struct instr_hdr {
        u16 code : 8;
        u16 operands : 2;
        u16 op1type : 2;
        u16 op2type : 2;
        u16 op3type : 2;
    };
    
    struct reg_hdr {
        bool floating : 1;
        u8 len : 2;
        u8 reg : 5;
    };
    
    // [location +- offset]
    struct mem_hdr {
        u8 len : 2; // Size of pointed at
        bool reg : 1; // Location is register
        u8 dis_type : 2; // Type of offset
        u8 imm_len : 2; // Length of immediate (Only one immediate possible, else optimized out)
        bool dis_signed : 1; // Offset is subtracted
    };
    
    struct imm_hdr {
        u8 len : 2;
        u8 _empty : 6;
    };
    
#pragma pack(pop)
    
    const dict<opcode> name_to_op {
        {"NOP", opcode::NOP},
        {"LOAD", opcode::LOAD},
        {"STOR", opcode::STOR},
        {"MOV", opcode::MOV},
        {"CPY", opcode::CPY},
        {"ZRO", opcode::ZRO},
        {"SET", opcode::SET},
        {"BRK", opcode::BRK},
        {"HLT", opcode::HLT},
        
        {"CZRO", opcode::CZRO},
        {"CNZR", opcode::CNZR},
        {"CEQ", opcode::CEQ},
        {"CNEQ", opcode::CNEQ},
        {"CBS", opcode::CBS},
        {"CBNS", opcode::CBNS},
        
        {"CLT", opcode::CLT},
        {"SCLT", opcode::SCLT},
        {"FCLT", opcode::FCLT},
        {"DCLT", opcode::DCLT},
        {"CLE", opcode::CLE},
        {"SCLE", opcode::SCLE},
        {"FCLE", opcode::FCLE},
        {"DCLE", opcode::DCLE},
        
        {"CGT", opcode::CGT},
        {"SCGT", opcode::SCGT},
        {"FCGT", opcode::FCGT},
        {"DCGT", opcode::DCGT},
        {"CGE", opcode::CGE},
        {"SCGE", opcode::SCGE},
        {"FCGE", opcode::FCGE},
        {"DCGE", opcode::DCGE},
        
        {"JMP", opcode::JMP},
        {"JMPR", opcode::JMPR},
        {"SJMPR", opcode::SJMPR},
        {"JCH", opcode::JCH},
        {"JNCH", opcode::JNCH},
        
        {"PUSH", opcode::PUSH},
        {"POP", opcode::POP},
        {"BTIN", opcode::BTIN},
        {"CALL", opcode::CALL},
        {"RET", opcode::RET},
        
        {"CSTU", opcode::CSTU},
        {"CSTF", opcode::CSTF},
        {"CSTD", opcode::CSTD},
        {"CUTS", opcode::CUTS},
        {"CUTF", opcode::CUTF},
        {"CUTD", opcode::CUTD},
        {"CFTS", opcode::CFTS},
        {"CFTU", opcode::CFTU},
        {"CFTD", opcode::CFTD},
        {"CDTS", opcode::CDTS},
        {"CDTU", opcode::CDTU},
        {"CDTF", opcode::CDTF},
        
        {"ADD", opcode::ADD},
        {"SADD", opcode::SADD},
        {"FADD", opcode::FADD},
        {"DADD", opcode::DADD},
        {"INC", opcode::INC},
        {"SINC", opcode::SINC},
        {"SUB", opcode::SUB},
        {"SSUB", opcode::SSUB},
        {"FSUB", opcode::FSUB},
        {"DSUB", opcode::DSUB},
        {"DEC", opcode::DEC},
        {"SDEC", opcode::SDEC},
        {"MUL", opcode::MUL},
        {"SMUL", opcode::SMUL},
        {"FMUL", opcode::FMUL},
        {"DMUL", opcode::DMUL},
        {"DIV", opcode::DIV},
        {"SDIV", opcode::SDIV},
        {"FDIV", opcode::FDIV},
        {"DDIV", opcode::DDIV},
        {"MOD", opcode::MOD},
        {"SMOD", opcode::SMOD},
        {"SABS", opcode::SABS},
        {"FABS", opcode::FABS},
        {"DABS", opcode::DABS},
        {"SNEG", opcode::SNEG},
        {"FNEG", opcode::FNEG},
        {"DNEG", opcode::DNEG},
        
        {"SHR", opcode::SHR},
        {"SSHR", opcode::SSHR},
        {"SHL", opcode::SHL},
        {"SSHL", opcode::SSHL},
        {"RTR", opcode::RTR},
        {"RTL", opcode::RTL},
        
        {"AND", opcode::AND},
        {"OR", opcode::OR},
        {"XOR", opcode::XOR},
        {"NOT", opcode::NOT},
        
        {"VAL", opcode::VAL},
        {"DB", opcode::DB},
        {"DBS", opcode::DBS},
        {"LBL", opcode::LBL},
        {"INVALID_CODE", opcode::INV},
    };
    
    const std::map<opcode, std::string> op_to_name{swap_key(name_to_op)};
    
    namespace format {
        namespace raw {
            constexpr u16 reg = 1u << 0;
            constexpr u16 imm = 1u << 1;
            constexpr u16 mem = 1u << 2;
            constexpr u16 u   = 1u << 3;
            constexpr u16 s   = 1u << 4;
            constexpr u16 f   = 1u << 5;
            constexpr u16 _8  = 1u << 6;
            constexpr u16 _16 = 1u << 7;
            constexpr u16 _32 = 1u << 8;
            constexpr u16 _64 = 1u << 9;
            
            constexpr u16 any_size = _8 | _16 | _32 | _64;
            constexpr u16 _int = u | s | any_size;
            constexpr u16 uint = u | any_size;
            constexpr u16 sint = s | any_size;
            constexpr u16 real = f | _32 | _64;
            constexpr u16 _float = f | _32;
            constexpr u16 _double = f | _64;
            
            constexpr u16 any_type = _int | real;
            constexpr u16 byte = u | _8;
            
            constexpr u16 any_reg = reg | any_type;
            constexpr u16 any_imm = imm | any_type;
            constexpr u16 any_mem = mem | any_type;
            
            constexpr u16 any_target = reg | imm | mem;
            constexpr u16 any = any_target | any_type;
            
            constexpr u16 mem_loc = any_target | uint;
            constexpr u16 any_uint = mem_loc;
            constexpr u16 any_sint = any_target | sint;
            constexpr u16 any_int  = any_target | _int;
            constexpr u16 any_float = any_target | _float;
            constexpr u16 any_double = any_target | _double;
            constexpr u16 any_real = any_target | real;
            constexpr u16 any_byte = any_target | byte;
        };
        
        struct operand {
            union {
                u16 raw;
                struct {
                    bool reg  : 1;
                    bool imm  : 1;
                    bool mem  : 1;
                    bool u    : 1;
                    bool s    : 1;
                    bool f    : 1;
                    bool _8   : 1;
                    bool _16  : 1;
                    
                    bool _32  : 1;
                    bool _64  : 1;
                    u8 _empty : 6;
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
        
        const std::map<opcode, std::vector<instruction>> get_formats();
    }
    
}

std::ostream& operator<<(std::ostream& os, nnasm::opcode code);
