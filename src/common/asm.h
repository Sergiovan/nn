#pragma once

#include <type_traits>
#include <map>
#include <vector>
#include "common/convenience.h"

namespace nnasm {
    namespace op {
        enum code {
            NOP, MOV, MVI, CPY, 
            ZRO, SET, BRK, HLT,
                        
            CZRO, CNZR, CEQ, CNEQ, 
            CLT, CLE, CGT, CGE, 
            CBS, CBNS,

            JMP, JMPR, JCH, JNCH,

            PUSH, POP, CALL, BTIN,
            RET,

            DRF, IDX,
            
            ADD, INC, SUB, DEC, 
            MUL, DIV, MOD, ABS, 
            NEG, SHR, SHL, RTR, 
            RTL, AND, OR, XOR, 
            NOT,

            VAL, DB, DBS, LBL,
            
            INVALID_CODE
        };
        
        enum operand {
            OP_NONE = 0,
            OP_REG = 1, OP_VAL = 2,
        };
        
        enum asm_type {
            ASM_TYPE_NONE = 0, 
            ASM_TYPE_SIGNED = 1 << 3,
            ASM_TYPE_FLOATING = 1 << 4,
            ASM_TYPE_QWORD = 1,
            ASM_TYPE_SQWORD = ASM_TYPE_SIGNED | ASM_TYPE_QWORD,
            ASM_TYPE_BYTE = 2,
            ASM_TYPE_SBYTE = ASM_TYPE_BYTE | ASM_TYPE_SIGNED,
            ASM_TYPE_WORD = 3,
            ASM_TYPE_SWORD = ASM_TYPE_WORD | ASM_TYPE_SIGNED,
            ASM_TYPE_DWORD = 4,
            ASM_TYPE_SDWORD = ASM_TYPE_DWORD | ASM_TYPE_SIGNED,
            ASM_TYPE_FLOAT = ASM_TYPE_FLOATING | 1,
            ASM_TYPE_DOUBLE = ASM_TYPE_FLOATING | 2,
            ASM_TYPE_INVALID = 0b11111,
        };
    }
    
    namespace format {
        enum operand_format {
            OP_NONE = 0,
            OP_REG = 1 << 0, OP_VAL = 1 << 1,
            
            OP_ANY = OP_REG | OP_VAL,
            
            OP_TYPE = OP_ANY,
            
            OP_FLAGS_NONE = 0,
            
            OP_FLAGS_UINT  = 1 << 2,
            OP_FLAGS_SINT  = 1 << 3,
            OP_FLAGS_INT   = OP_FLAGS_UINT | OP_FLAGS_SINT,
            OP_FLAGS_FLOAT = 1 << 4,
            
            OP_FLAGS = OP_FLAGS_UINT | OP_FLAGS_SINT | OP_FLAGS_FLOAT,
            
        };
        
        constexpr operand_format operator|(operand_format lhs, operand_format rhs) {
            return static_cast<operand_format> (
                static_cast<std::underlying_type_t<operand_format>>(lhs) |
                static_cast<std::underlying_type_t<operand_format>>(rhs)
            );
        }
        
        struct instruction_format {
            op::code code;
            operand_format op1{OP_NONE};
            operand_format op2{OP_NONE};
            operand_format op3{OP_NONE};
        };
        
        namespace {            
            inline const std::map<op::code, std::vector<instruction_format>> create_formats() {
                using opcode = op::code;
                using opformat = operand_format;
                
                std::map<op::code, std::vector<instruction_format>> ret{};
                
                auto add = [&ret](opcode code, opformat op1 = OP_NONE, opformat op2 = OP_NONE, opformat op3 = OP_NONE, opformat f = OP_FLAGS_NONE) {
                    ret[code].push_back({code, op1 | f, op2 | f, op3 | f});
                };
                
                auto add_any = [&ret](opcode code, int anies = 1, opformat f = OP_FLAGS_NONE) {
                    instruction_format inf{code};
                    switch (anies) {
                        case 3:
                            inf.op3 = OP_ANY | f;
                            [[fallthrough]];
                        case 2:
                            inf.op2 = OP_ANY | f;
                            [[fallthrough]];
                        case 1:
                            inf.op1 = OP_ANY | f;
                            [[fallthrough]];
                        default:
                            break;
                    }
                    ret[code].push_back(inf);
                };
                
                auto add_1op_arithmetic = [&ret](opcode code, opformat f = OP_FLAGS_NONE) {
                    ret[code].push_back({code, OP_REG | f});
                    ret[code].push_back({code, OP_ANY | f, OP_REG | f});
                };
                
                auto add_2op_arithmetic = [&ret](opcode code, opformat f = OP_FLAGS_NONE) {
                    ret[code].push_back({code, OP_ANY | f, OP_REG | f});
                    ret[code].push_back({code, OP_ANY | f, OP_ANY | f, OP_REG | f});
                };
                
                constexpr opformat OP_ANY_SINT = OP_ANY | OP_FLAGS_SINT;
                constexpr opformat OP_ANY_UINT = OP_ANY | OP_FLAGS_UINT;
                
                add(op::NOP); // NOP - No operation
                add(op::MOV, OP_ANY, OP_REG); // MOV - Move
                add(op::MOV, OP_ANY, OP_VAL | OP_FLAGS_INT);
                add(op::MVI, OP_ANY_SINT, OP_REG); // MVI - Move indirect
                add(op::MVI, OP_ANY_SINT, OP_ANY_SINT, OP_REG);
                add(op::CPY, OP_REG, OP_REG, OP_ANY_UINT); // CPY - Copy
                add(op::ZRO, OP_ANY_SINT, OP_ANY_UINT); // ZRO - Zero
                add(op::SET, OP_ANY, OP_REG, OP_ANY_UINT); // SET - Set
                add(op::BRK); // BRK - Break
                add(op::HLT); // HLT - Halt 
                
                add_any(op::CZRO); // CZRO - Check zero
                add_any(op::CNZR); // CNZR - Check not zero
                add_any(op::CEQ, 2); // CEQ - Check equal
                add_any(op::CNEQ, 2); // CNEQ - Check not equal
                add_any(op::CLT, 2); // CLT - Check less than
                add_any(op::CLE, 2); // CLE - Check less or equal than
                add_any(op::CGT, 2); // CGT - Check greater than
                add_any(op::CGE, 2); // CGE - Check greater or equal than
                add(op::CBS, OP_ANY, OP_ANY_UINT); // CBS - Check bit set
                add(op::CBNS, OP_ANY, OP_ANY_UINT); // CBNS - Check bit not set
                
                add(op::JMP, OP_ANY | OP_FLAGS_INT); // JMP - Jump
                add(op::JMPR, OP_ANY | OP_FLAGS_INT); // JMPR - Jump relative
                add(op::JCH, OP_ANY | OP_FLAGS_INT); // JCH - Jump if check
                add(op::JNCH, OP_ANY | OP_FLAGS_INT); // JNCH - Jump if not check
                
                add(op::PUSH, OP_VAL); // PUSH - Push
                add(op::PUSH, OP_REG);
                add(op::PUSH, OP_ANY, OP_ANY_UINT); 
                add(op::POP, OP_VAL | OP_FLAGS_UINT); // POP - Pop
                add(op::POP, OP_ANY, OP_ANY_UINT);
                add(op::POP, OP_REG);
                add(op::BTIN, OP_ANY_UINT); // BTIN - Built-in
                add(op::CALL, OP_ANY | OP_FLAGS_INT); // CALL - Call
                add(op::RET); // RET - Return
                
                add(op::DRF, OP_REG); // DRF - Dereference
                add(op::DRF, OP_ANY_SINT, OP_REG);
                add(op::IDX, OP_ANY_SINT, OP_REG); // IDX - Index
                add(op::IDX, OP_ANY_SINT, OP_REG, OP_REG); 
                
                add_2op_arithmetic(op::ADD); // ADD - Add
                add_1op_arithmetic(op::INC); // INC - Increment
                add_2op_arithmetic(op::SUB); // SUB - Subtract
                add_1op_arithmetic(op::DEC); // DEC - Decrement
                add_2op_arithmetic(op::MUL); // MUL - Multiply
                add_2op_arithmetic(op::DIV); // DIV - Divide
                add_2op_arithmetic(op::MOD, OP_FLAGS_INT); // MOD - Modulo
                add_1op_arithmetic(op::ABS); // ABS - Absolute value
                add_1op_arithmetic(op::NEG); // NEG - Negate value
                add(op::SHR, OP_ANY_UINT, OP_REG | OP_FLAGS_INT); // SHR - Shift right
                add(op::SHR, OP_ANY_UINT, OP_ANY | OP_FLAGS_INT, OP_REG | OP_FLAGS_INT);
                add(op::SHL, OP_ANY_UINT, OP_REG | OP_FLAGS_INT); // SHL - Shift left
                add(op::SHL, OP_ANY_UINT, OP_ANY | OP_FLAGS_INT, OP_REG | OP_FLAGS_INT);
                add(op::RTR, OP_ANY_UINT, OP_REG | OP_FLAGS_INT); // RTR - Rotate right
                add(op::RTR, OP_ANY_UINT, OP_ANY | OP_FLAGS_INT, OP_REG | OP_FLAGS_INT);
                add(op::RTL, OP_ANY_UINT, OP_REG | OP_FLAGS_INT); // RTL - Rotate left
                add(op::RTL, OP_ANY_UINT, OP_ANY | OP_FLAGS_INT, OP_REG | OP_FLAGS_INT);
                add_2op_arithmetic(op::AND, OP_FLAGS_INT); // AND - And
                add_2op_arithmetic(op::OR, OP_FLAGS_INT); // OR  - Or
                add_2op_arithmetic(op::XOR, OP_FLAGS_INT); // XOR - Xor
                add_1op_arithmetic(op::NOT, OP_FLAGS_INT); // NOT - Not
                
                return ret;
            }
        }
        
        static const std::map<op::code, std::vector<instruction_format>> instructions{create_formats()};
    }
    
    struct instruction_code {
        struct operand {
            union {
                u16 raw;
                struct {
                    u16 optype : 2;
                    u16 valtype : 5;
                    u16 number : 4; // Only used in registers
                    u16 deref  : 5; // If dereference, value of dereference
                };
            };
        };
    
        union {
            u64 raw;
            struct {
                u16 opcode;
                operand ops[3];
            };
        };
    };
    
    struct instruction {
        instruction_code code;
        u64 values[3];
    };
    
    static const dict<op::code> name_to_op {
        {"NOP", op::NOP}, {"MOV", op::MOV}, {"MVI", op::MVI}, {"CPY", op::CPY}, 
        {"ZRO", op::ZRO}, {"SET", op::SET}, {"BRK", op::BRK}, {"HLT", op::HLT},
                    
        {"CZRO", op::CZRO}, {"CNZR", op::CNZR}, {"CEQ", op::CEQ}, {"CNEQ", op::CNEQ}, 
        {"CLT", op::CLT}, {"CLE", op::CLE}, {"CGT", op::CGT}, {"CGE", op::CGE}, 
        {"CBS", op::CBS}, {"CBNS", op::CBNS},

        {"JMP", op::JMP}, {"JMPR", op::JMPR}, {"JCH", op::JCH}, {"JNCH", op::JNCH},

        {"PUSH", op::PUSH}, {"POP", op::POP}, {"CALL", op::CALL}, {"BTIN", op::BTIN},
        {"RET", op::RET},

        {"DRF", op::DRF}, {"IDX", op::IDX},
        
        {"ADD", op::ADD}, {"INC", op::INC}, {"SUB", op::SUB}, {"DEC", op::DEC}, 
        {"MUL", op::MUL}, {"DIV", op::DIV}, {"MOD", op::MOD}, {"ABS", op::ABS}, 
        {"NEG", op::NEG}, {"SHR", op::SHR}, {"SHL", op::SHL}, {"RTR", op::RTR}, 
        {"RTL", op::RTL}, {"AND", op::AND}, {"OR", op::OR}, {"XOR", op::XOR}, 
        {"NOT", op::NOT},

        {"VAL", op::VAL}, {"DB", op::DB}, {"DBS", op::DBS}, {"LBL", op::LBL},
    };
    
    static const std::map<op::code, std::string> op_to_name{swap_key(name_to_op)};
}
