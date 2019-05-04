#pragma once

#include <type_traits>
#include <map>
#include <vector>
#include <iostream>
#include "common/convenience.h"

namespace nnasm {
    enum class opcode : u8 {
        NOP, LOAD, STOR, MOV, CPY, ZRO, SET, BRK, HLT,
        CZRO, CNZR, CEQ, CNEQ, CBS, CBNS, 
        CLT, SCLT, FCLT, DCLT, CLE, SCLE, FCLE, DCLE, 
        CGT, SCGT, FCGT, DCGT, CGE, SCGE, FCGE, DCGE, 
        JMP, JMPR, JCH, JNCH, PUSH, POP, BTIN, CALL, RET,
        CSTU, CSTF, CSTD, CUTS, CUTF, CUTD, CFTS, CFTU, CFTD, CDTS, CDTU, CDTF, 
        ADD, SADD, FADD, DADD, INC, SINC, SUB, SSUB, FSUB, DSUB, DEC, SDEC,
        MUL, SMUL, FMUL, DMUL, DIV, SDIV, FDIV, DDIV, MOD, SMOD,
        SABS, FABS, DABS, SNEG, FNEG, DNEG, 
        SHR, SSHR, SHL, SSHL, RTR, SRTR, RTL, SRTL, 
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
    
    struct mem_hdr {
        u8 len : 2;
        u8 dis_type : 2;
        u8 dis_len : 2;
        bool dis_signed : 1;
        u8 _empty : 1;
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
        {"SRTR", opcode::SRTR},
        {"RTL", opcode::RTL},
        {"SRTL", opcode::SRTL},
        
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
                    operand op1;
                    operand op2;
                    operand op3;
                    u16 _empty;
                };
                u64 raw;
            };
        };
        
        const std::map<opcode, std::vector<instruction>> get_formats();
    }
    
}

std::ostream& operator<<(std::ostream& os, nnasm::opcode code);

// namespace nnasm {
//     namespace op {
//         enum code {
//             NOP, MOV, MVI, CPY, 
//             ZRO, SET, BRK, HLT,
//                         
//             CZRO, CNZR, CEQ, CNEQ, 
//             CLT, CLE, CGT, CGE, 
//             CBS, CBNS,
// 
//             JMP, JMPR, JCH, JNCH,
// 
//             PUSH, POP, CALL, BTIN,
//             RET,
// 
//             DRF, IDX,
//             
//             ADD, INC, SUB, DEC, 
//             MUL, DIV, MOD, ABS, 
//             NEG, SHR, SHL, RTR, 
//             RTL, AND, OR, XOR, 
//             NOT,
// 
//             VAL, DB, DBS, LBL,
//             
//             INVALID_CODE
//         };
//         
//         enum operand {
//             OP_NONE = 0,
//             OP_REG = 1, OP_VAL = 2,
//         };
//         
//         enum asm_type {
//             ASM_TYPE_NONE = 0, 
//             ASM_TYPE_SIGNED = 1 << 3,
//             ASM_TYPE_FLOATING = 1 << 4,
//             ASM_TYPE_QWORD = 1,
//             ASM_TYPE_SQWORD = ASM_TYPE_SIGNED | ASM_TYPE_QWORD,
//             ASM_TYPE_BYTE = 2,
//             ASM_TYPE_SBYTE = ASM_TYPE_BYTE | ASM_TYPE_SIGNED,
//             ASM_TYPE_WORD = 3,
//             ASM_TYPE_SWORD = ASM_TYPE_WORD | ASM_TYPE_SIGNED,
//             ASM_TYPE_DWORD = 4,
//             ASM_TYPE_SDWORD = ASM_TYPE_DWORD | ASM_TYPE_SIGNED,
//             ASM_TYPE_FLOAT = ASM_TYPE_FLOATING | 1,
//             ASM_TYPE_DOUBLE = ASM_TYPE_FLOATING | 2,
//             ASM_TYPE_INVALID = 0b11111,
//         };
//     }
//     
//     namespace format {
//         enum operand_format {
//             OP_NONE = 0,
//             OP_REG = 1 << 0, OP_VAL = 1 << 1,
//             
//             OP_ANY = OP_REG | OP_VAL,
//             
//             OP_TYPE = OP_ANY,
//             
//             OP_FLAGS_NONE = 0,
//             
//             OP_FLAGS_UINT  = 1 << 2,
//             OP_FLAGS_SINT  = 1 << 3,
//             OP_FLAGS_INT   = OP_FLAGS_UINT | OP_FLAGS_SINT,
//             OP_FLAGS_FLOAT = 1 << 4,
//             
//             OP_FLAGS = OP_FLAGS_UINT | OP_FLAGS_SINT | OP_FLAGS_FLOAT,
//             
//         };
//         
//         constexpr operand_format operator|(operand_format lhs, operand_format rhs) {
//             return static_cast<operand_format> (
//                 static_cast<std::underlying_type_t<operand_format>>(lhs) |
//                 static_cast<std::underlying_type_t<operand_format>>(rhs)
//             );
//         }
//         
//         struct instruction_format {
//             op::code code;
//             operand_format op1{OP_NONE};
//             operand_format op2{OP_NONE};
//             operand_format op3{OP_NONE};
//         };
//         
//         namespace {            
//             inline const std::map<op::code, std::vector<instruction_format>> create_formats() {
//                 using opcode = op::code;
//                 using opformat = operand_format;
//                 
//                 std::map<op::code, std::vector<instruction_format>> ret{};
//                 
//                 auto add = [&ret](opcode code, opformat op1 = OP_NONE, opformat op2 = OP_NONE, opformat op3 = OP_NONE, opformat f = OP_FLAGS_NONE) {
//                     ret[code].push_back({code, op1 | f, op2 | f, op3 | f});
//                 };
//                 
//                 auto add_any = [&ret](opcode code, int anies = 1, opformat f = OP_FLAGS_NONE) {
//                     instruction_format inf{code};
//                     switch (anies) {
//                         case 3:
//                             inf.op3 = OP_ANY | f;
//                             [[fallthrough]];
//                         case 2:
//                             inf.op2 = OP_ANY | f;
//                             [[fallthrough]];
//                         case 1:
//                             inf.op1 = OP_ANY | f;
//                             [[fallthrough]];
//                         default:
//                             break;
//                     }
//                     ret[code].push_back(inf);
//                 };
//                 
//                 auto add_1op_arithmetic = [&ret](opcode code, opformat f = OP_FLAGS_NONE) {
//                     ret[code].push_back({code, OP_REG | f});
//                     ret[code].push_back({code, OP_ANY | f, OP_REG | f});
//                 };
//                 
//                 auto add_2op_arithmetic = [&ret](opcode code, opformat f = OP_FLAGS_NONE) {
//                     ret[code].push_back({code, OP_ANY | f, OP_REG | f});
//                     ret[code].push_back({code, OP_ANY | f, OP_ANY | f, OP_REG | f});
//                 };
//                 
//                 constexpr opformat OP_VAL_INT  = OP_VAL | OP_FLAGS_INT;
//                 constexpr opformat OP_VAL_UINT  = OP_VAL | OP_FLAGS_SINT;
//                 constexpr opformat OP_VAL_SINT  = OP_VAL | OP_FLAGS_UINT;
//                 constexpr opformat OP_ANY_INT  = OP_ANY | OP_FLAGS_INT;
//                 constexpr opformat OP_ANY_SINT = OP_ANY | OP_FLAGS_SINT;
//                 constexpr opformat OP_ANY_UINT = OP_ANY | OP_FLAGS_UINT;
//                 
//                 add(op::NOP); // NOP - No operation
//                 add(op::MOV, OP_ANY, OP_REG); // MOV - Move
//                 add(op::MOV, OP_ANY, OP_VAL_INT);
//                 add(op::MVI, OP_ANY_INT, OP_REG); // MVI - Move indirect
//                 add(op::MVI, OP_ANY_INT, OP_ANY_INT, OP_REG);
//                 add(op::CPY, OP_ANY_INT, OP_ANY_INT, OP_ANY_UINT); // CPY - Copy
//                 add(op::ZRO, OP_ANY_INT, OP_ANY_UINT); // ZRO - Zero
//                 add(op::SET, OP_ANY, OP_REG, OP_ANY_UINT); // SET - Set
//                 add(op::BRK); // BRK - Break
//                 add(op::HLT); // HLT - Halt 
//                 
//                 add_any(op::CZRO); // CZRO - Check zero
//                 add_any(op::CNZR); // CNZR - Check not zero
//                 add_any(op::CEQ, 2); // CEQ - Check equal
//                 add_any(op::CNEQ, 2); // CNEQ - Check not equal
//                 add_any(op::CLT, 2); // CLT - Check less than
//                 add_any(op::CLE, 2); // CLE - Check less or equal than
//                 add_any(op::CGT, 2); // CGT - Check greater than
//                 add_any(op::CGE, 2); // CGE - Check greater or equal than
//                 add(op::CBS, OP_ANY, OP_ANY_UINT); // CBS - Check bit set
//                 add(op::CBNS, OP_ANY, OP_ANY_UINT); // CBNS - Check bit not set
//                 
//                 add(op::JMP, OP_ANY_INT); // JMP - Jump
//                 add(op::JMPR, OP_ANY_INT); // JMPR - Jump relative
//                 add(op::JCH, OP_ANY_INT); // JCH - Jump if check
//                 add(op::JNCH, OP_ANY_INT); // JNCH - Jump if not check
//                 
//                 add(op::PUSH, OP_VAL); // PUSH - Push
//                 add(op::PUSH, OP_REG);
//                 add(op::PUSH, OP_ANY, OP_ANY_UINT); 
//                 add(op::POP, OP_VAL_UINT); // POP - Pop
//                 add(op::POP, OP_ANY, OP_ANY_UINT);
//                 add(op::POP, OP_REG);
//                 add(op::BTIN, OP_ANY_UINT); // BTIN - Built-in
//                 add(op::CALL, OP_ANY | OP_FLAGS_INT); // CALL - Call
//                 add(op::RET); // RET - Return
//                 
//                 add(op::DRF, OP_REG); // DRF - Dereference
//                 add(op::DRF, OP_ANY_SINT, OP_REG);
//                 add(op::IDX, OP_ANY_SINT, OP_REG); // IDX - Index
//                 add(op::IDX, OP_ANY_SINT, OP_REG, OP_REG); 
//                 
//                 add_2op_arithmetic(op::ADD); // ADD - Add
//                 add_1op_arithmetic(op::INC); // INC - Increment
//                 add_2op_arithmetic(op::SUB); // SUB - Subtract
//                 add_1op_arithmetic(op::DEC); // DEC - Decrement
//                 add_2op_arithmetic(op::MUL); // MUL - Multiply
//                 add_2op_arithmetic(op::DIV); // DIV - Divide
//                 add_2op_arithmetic(op::MOD, OP_FLAGS_INT); // MOD - Modulo
//                 add_1op_arithmetic(op::ABS); // ABS - Absolute value
//                 add_1op_arithmetic(op::NEG); // NEG - Negate value
//                 add(op::SHR, OP_ANY_UINT, OP_REG | OP_FLAGS_INT); // SHR - Shift right
//                 add(op::SHR, OP_ANY_UINT, OP_ANY | OP_FLAGS_INT, OP_REG | OP_FLAGS_INT);
//                 add(op::SHL, OP_ANY_UINT, OP_REG | OP_FLAGS_INT); // SHL - Shift left
//                 add(op::SHL, OP_ANY_UINT, OP_ANY | OP_FLAGS_INT, OP_REG | OP_FLAGS_INT);
//                 add(op::RTR, OP_ANY_UINT, OP_REG | OP_FLAGS_INT); // RTR - Rotate right
//                 add(op::RTR, OP_ANY_UINT, OP_ANY | OP_FLAGS_INT, OP_REG | OP_FLAGS_INT);
//                 add(op::RTL, OP_ANY_UINT, OP_REG | OP_FLAGS_INT); // RTL - Rotate left
//                 add(op::RTL, OP_ANY_UINT, OP_ANY | OP_FLAGS_INT, OP_REG | OP_FLAGS_INT);
//                 add_2op_arithmetic(op::AND, OP_FLAGS_INT); // AND - And
//                 add_2op_arithmetic(op::OR, OP_FLAGS_INT); // OR  - Or
//                 add_2op_arithmetic(op::XOR, OP_FLAGS_INT); // XOR - Xor
//                 add_1op_arithmetic(op::NOT, OP_FLAGS_INT); // NOT - Not
//                 
//                 return ret;
//             }
//         }
//         
//         static const std::map<op::code, std::vector<instruction_format>> instructions{create_formats()};
//     }
//     
//     struct instruction_code {
//         struct operand {
//             union {
//                 u16 raw;
//                 struct {
//                     u16 optype : 2;
//                     u16 valtype : 5;
//                     u16 number : 4; // Only used in registers
//                     u16 deref  : 5; // If dereference, value of dereference
//                 };
//             };
//         };
//     
//         union {
//             u64 raw;
//             struct {
//                 u16 opcode;
//                 operand ops[3];
//             };
//         };
//     };
//     
//     struct instruction {
//         instruction_code code;
//         u64 values[3];
//     };
//     
//     static const dict<op::code> name_to_op {
//         {"NOP", op::NOP}, {"MOV", op::MOV}, {"MVI", op::MVI}, {"CPY", op::CPY}, 
//         {"ZRO", op::ZRO}, {"SET", op::SET}, {"BRK", op::BRK}, {"HLT", op::HLT},
//                     
//         {"CZRO", op::CZRO}, {"CNZR", op::CNZR}, {"CEQ", op::CEQ}, {"CNEQ", op::CNEQ}, 
//         {"CLT", op::CLT}, {"CLE", op::CLE}, {"CGT", op::CGT}, {"CGE", op::CGE}, 
//         {"CBS", op::CBS}, {"CBNS", op::CBNS},
// 
//         {"JMP", op::JMP}, {"JMPR", op::JMPR}, {"JCH", op::JCH}, {"JNCH", op::JNCH},
// 
//         {"PUSH", op::PUSH}, {"POP", op::POP}, {"CALL", op::CALL}, {"BTIN", op::BTIN},
//         {"RET", op::RET},
// 
//         {"DRF", op::DRF}, {"IDX", op::IDX},
//         
//         {"ADD", op::ADD}, {"INC", op::INC}, {"SUB", op::SUB}, {"DEC", op::DEC}, 
//         {"MUL", op::MUL}, {"DIV", op::DIV}, {"MOD", op::MOD}, {"ABS", op::ABS}, 
//         {"NEG", op::NEG}, {"SHR", op::SHR}, {"SHL", op::SHL}, {"RTR", op::RTR}, 
//         {"RTL", op::RTL}, {"AND", op::AND}, {"OR", op::OR}, {"XOR", op::XOR}, 
//         {"NOT", op::NOT},
// 
//         {"VAL", op::VAL}, {"DB", op::DB}, {"DBS", op::DBS}, {"LBL", op::LBL},
//     };
//     
//     static const std::map<op::code, std::string> op_to_name{swap_key(name_to_op)};
// }
