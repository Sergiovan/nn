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
                        
            CZRO, CNZR, CEQ,
            CNEQ, CLT, CLTS, 
            CLTF, CLE, CLES,
            CLEF, CGT, CGTS, 
            CGTF, CGE, CGES, 
            CGEF, CBS, CBNS,

            JMP, JMPR, JCH, JNCH,

            PUSH, POP, CALL, RET,

            DRF, IDX,
            
            ADD, INC, SUB, DEC, 
            MUL, DIV, MOD, ABS, 
            NEG, SHR, SHL, RTR, 
            RTL, AND, OR, XOR, 
            NOT,

            ADDS, INCS, SUBS, 
            DECS, MULS, DIVS,
            MODS, ABSS, NEGS, 
            SHRS, SHLS,

            ADDF, SUBF, MULF, 
            DIVF, MODF, ABSF, 
            NEGF, 

            VAL, DB, DBS, LBL,
            
            INVALID_CODE
        };
        
        enum operand {
            OP_NONE = 0,
            OP_REG = 1, OP_VAL = 2, OP_ADDR = 3
        };
        
        enum reg {
            REG_SIGNED = 1 << 5,
            REG_QWORD = 0,
            REG_SQWORD = REG_SIGNED | REG_QWORD,
            REG_BYTE = 1 << 0,
            REG_SBYTE = REG_BYTE | REG_SIGNED,
            REG_WORD = 1 << 1,
            REG_SWORD = REG_WORD | REG_SIGNED,
            REG_DWORD = 1 << 2,
            REG_SDWORD = REG_DWORD | REG_SIGNED,
            REG_FLOAT = 1 << 3,
            REG_DOUBLE = 1 << 4,
        };
    }
    
    namespace format {
        enum operand_format {
            OP_NONE = 0,
            OP_REG = 1 << 0, OP_VAL = 1 << 1, OP_ADDR = 1 << 2,
            
            OP_REG_VAL = OP_REG | OP_VAL,
            OP_REG_ADDR = OP_REG | OP_ADDR,
            OP_VAL_ADDR = OP_VAL | OP_ADDR,
            OP_ANY = OP_REG | OP_VAL | OP_ADDR,
            
            OP_TYPE = OP_ANY,
            
            OP_FLAGS_NONE = 0,
            
            OP_FLAGS_UINT  = 1 << 3,
            OP_FLAGS_SINT  = 1 << 4,
            OP_FLAGS_INT   = OP_FLAGS_UINT | OP_FLAGS_SINT,
            OP_FLAGS_FLOAT = 1 << 5,
            
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
                
                auto add_1op_arithmetic = [&ret](opcode code, opformat f = OP_FLAGS_UINT) {
                    ret[code].push_back({code, OP_REG_ADDR | f});
                    ret[code].push_back({code, OP_ANY | f, OP_REG_ADDR | (f == OP_FLAGS_FLOAT ? OP_FLAGS_FLOAT : OP_FLAGS_INT)});
                };
                
                auto add_2op_arithmetic = [&ret](opcode code, opformat f = OP_FLAGS_UINT) {
                    ret[code].push_back({code, OP_ANY | f, OP_REG_ADDR | f});
                    ret[code].push_back({code, OP_ANY | f, OP_ANY | f, OP_REG_ADDR | (f == OP_FLAGS_FLOAT ? OP_FLAGS_FLOAT : OP_FLAGS_INT)});
                };
                
                constexpr opformat OP_ANY_SINT = OP_ANY | OP_FLAGS_SINT;
                constexpr opformat OP_ANY_UINT = OP_ANY | OP_FLAGS_UINT;
                
                add(op::NOP); // NOP - No operation
                add(op::MOV, OP_ANY, OP_REG_ADDR); // MOV - Move
                add(op::MVI, OP_ANY_SINT, OP_REG_ADDR); // MVI - Move indirect
                add(op::MVI, OP_ANY_SINT, OP_ANY_SINT, OP_REG_ADDR);
                add(op::CPY, OP_REG_ADDR, OP_REG_ADDR, OP_ANY_UINT); // CPY - Copy
                add(op::ZRO, OP_REG_ADDR, OP_ANY_UINT); // ZRO - Zero
                add(op::SET, OP_ANY, OP_REG_ADDR, OP_ANY_UINT); // SET - Set
                add(op::BRK); // BRK - Break
                add(op::HLT); // HLT - Halt 
                
                add_any(op::CZRO); // CZRO - Check zero
                add_any(op::CNZR); // CNZR - Check not zero
                add_any(op::CEQ, 2); // CEQ - Check equal
                add_any(op::CNEQ, 2); // CNEQ - Check not equal
                add_any(op::CLT, 2, OP_FLAGS_UINT); // CLT - Check less than
                add_any(op::CLTS, 2, OP_FLAGS_SINT); // CLTS - Check less than signed
                add_any(op::CLTF, 2, OP_FLAGS_FLOAT); // CLTF - Check less than float
                add_any(op::CLE, 2, OP_FLAGS_UINT); // CLE - Check less or equal than
                add_any(op::CLES, 2, OP_FLAGS_SINT); // CLES - Check less or equal than signed
                add_any(op::CLEF, 2, OP_FLAGS_FLOAT); // CLEF - Check less or equal than float
                add_any(op::CGT, 2, OP_FLAGS_UINT); // CGT - Check greater than
                add_any(op::CGTS, 2, OP_FLAGS_SINT); // CGTS - Check greater than signed
                add_any(op::CGTF, 2, OP_FLAGS_FLOAT); // CGTF - Check greater than float
                add_any(op::CGE, 2, OP_FLAGS_UINT); // CGE - Check greater or equal than
                add_any(op::CGES, 2, OP_FLAGS_SINT); // CGES - Check greater or equal than signed
                add_any(op::CGEF, 2, OP_FLAGS_FLOAT); // CGEF - Check greater or equal than float
                add(op::CBS, OP_ANY, OP_ANY_UINT); // CBS - Check bit set
                add(op::CBNS, OP_ANY, OP_ANY_UINT); // CBNS - Check bit not set
                
                add(op::JMP, OP_ANY_UINT); // JMP - Jump
                add(op::JMPR, OP_ANY_SINT); // JMPR - Jump relative
                add(op::JCH, OP_ANY_UINT); // JCH - Jump if check
                add(op::JNCH, OP_ANY_UINT); // JNCH - Jump if not check
                
                add(op::PUSH, OP_VAL); // PUSH - Push
                add(op::PUSH, OP_REG);
                add(op::PUSH, OP_ADDR, OP_ANY_UINT); 
                add(op::POP, OP_VAL_ADDR | OP_FLAGS_UINT); // POP - Pop
                add(op::POP, OP_ADDR, OP_ANY_UINT);
                add(op::POP, OP_REG);
                add(op::CALL, OP_VAL | OP_FLAGS_UINT); // CALL - Call
                add(op::CALL, OP_REG_ADDR);
                add(op::RET); // RET - Return
                
                add(op::DRF, OP_REG); // DRF - Dereference
                add(op::DRF, OP_ANY_UINT, OP_REG);
                add(op::IDX, OP_ANY, OP_REG); // IDX - Index
                add(op::IDX, OP_ANY, OP_REG, OP_REG); 
                
                add_2op_arithmetic(op::ADD); // ADD - Add
                add_2op_arithmetic(op::ADDS, OP_FLAGS_SINT);
                add_2op_arithmetic(op::ADDF, OP_FLAGS_FLOAT);
                add_1op_arithmetic(op::INC); // INC - Increment
                add_1op_arithmetic(op::INCS, OP_FLAGS_SINT);
                add_2op_arithmetic(op::SUB); // SUB - Subtract
                add_2op_arithmetic(op::SUBS, OP_FLAGS_SINT);
                add_2op_arithmetic(op::SUBF, OP_FLAGS_FLOAT);
                add_1op_arithmetic(op::DEC); // DEC - Decrement
                add_1op_arithmetic(op::DECS, OP_FLAGS_SINT);
                add_2op_arithmetic(op::MUL); // MUL - Multiply
                add_2op_arithmetic(op::MULS, OP_FLAGS_SINT);
                add_2op_arithmetic(op::MULF, OP_FLAGS_FLOAT);
                add_2op_arithmetic(op::DIV); // DIV - Divide
                add_2op_arithmetic(op::DIVS, OP_FLAGS_SINT);
                add_2op_arithmetic(op::DIVF, OP_FLAGS_FLOAT);
                add_2op_arithmetic(op::MOD); // MOD - Modulo
                add_2op_arithmetic(op::MODS, OP_FLAGS_SINT);
                add_2op_arithmetic(op::MODF, OP_FLAGS_FLOAT);
                add_1op_arithmetic(op::ABS); // ABS - Absolute value
                add_1op_arithmetic(op::ABSS, OP_FLAGS_SINT);
                add_1op_arithmetic(op::ABSF, OP_FLAGS_FLOAT);
                // No single op unsigned NEG -- Makes no dang sense
                add(op::NEG, OP_ANY | OP_FLAGS_UINT, OP_REG_ADDR | OP_FLAGS_INT); // NEG - Negate
                add_1op_arithmetic(op::NEGS, OP_FLAGS_SINT);
                add_1op_arithmetic(op::NEGF, OP_FLAGS_FLOAT);
                add_2op_arithmetic(op::SHR); // SHR - Shift right
                add(op::SHRS, OP_ANY_UINT, OP_REG_ADDR | OP_FLAGS_SINT);
                add(op::SHRS, OP_ANY_UINT, OP_ANY_SINT, OP_REG_ADDR | OP_FLAGS_INT);
                add_2op_arithmetic(op::SHL); // SHL - Shift left
                add(op::RTR, OP_ANY_UINT, OP_REG_ADDR | OP_FLAGS_INT); // RTR - Rotate right
                add(op::RTR, OP_ANY_UINT, OP_ANY | OP_FLAGS_INT, OP_REG_ADDR | OP_FLAGS_INT);
                add(op::RTL, OP_ANY_UINT, OP_REG_ADDR | OP_FLAGS_INT); // RTL - Rotate left
                add(op::RTL, OP_ANY_UINT, OP_ANY | OP_FLAGS_INT, OP_REG_ADDR | OP_FLAGS_INT);
                add_2op_arithmetic(op::AND, OP_FLAGS_INT); // AND - And
                add_2op_arithmetic(op::OR, OP_FLAGS_INT); // OR  - Or
                add_2op_arithmetic(op::XOR, OP_FLAGS_INT); // XOR - Xor
                add_1op_arithmetic(op::NOT, OP_FLAGS_INT); // NOT - Not
                
                return ret;
            }
        }
        
        static const std::map<op::code, std::vector<instruction_format>> instructions{create_formats()};
    }
    
    struct instruction {
        union {
            struct {
                uint16_t code : 10;
                uint16_t op1  : 2;
                uint16_t op2  : 2;
                uint16_t op3  : 2;
            };
            uint16_t raw;
        };
    };
    
    static const dict<op::code> name_to_op {
        {"NOP", op::NOP}, {"MOV", op::MOV}, {"MVI", op::MVI}, {"CPY", op::CPY}, 
        {"ZRO", op::ZRO}, {"SET", op::SET}, {"BRK", op::BRK}, {"HLT", op::HLT},
                    
        {"CZRO", op::CZRO}, {"CNZR", op::CNZR}, {"CEQ", op::CEQ},
        {"CNEQ", op::CNEQ}, {"CLT", op::CLT}, {"CLTS", op::CLTS}, 
        {"CLTF", op::CLTF}, {"CLE", op::CLE}, {"CLES", op::CLES},
        {"CLEF", op::CLEF}, {"CGT", op::CGT}, {"CGTS", op::CGTS}, 
        {"CGTF", op::CGTF}, {"CGE", op::CGE}, {"CGES", op::CGES}, 
        {"CGEF", op::CGEF}, {"CBS", op::CBS}, {"CBNS", op::CBNS},

        {"JMP", op::JMP}, {"JMPR", op::JMPR}, {"JCH", op::JCH}, {"JNCH", op::JNCH},

        {"PUSH", op::PUSH}, {"POP", op::POP}, {"CALL", op::CALL}, {"RET", op::RET},

        {"DRF", op::DRF}, {"IDX", op::IDX},
        
        {"ADD", op::ADD}, {"INC", op::INC}, {"SUB", op::SUB}, {"DEC", op::DEC}, 
        {"MUL", op::MUL}, {"DIV", op::DIV}, {"MOD", op::MOD}, {"ABS", op::ABS}, 
        {"NEG", op::NEG}, {"SHR", op::SHR}, {"SHL", op::SHL}, {"RTR", op::RTR}, 
        {"RTL", op::RTL}, {"AND", op::AND}, {"OR", op::OR}, {"XOR", op::XOR}, 
        {"NOT", op::NOT},

        {"ADDS", op::ADDS}, {"INCS", op::INCS}, {"SUBS", op::SUBS}, 
        {"DECS", op::DECS}, {"MULS", op::MULS}, {"DIVS", op::DIVS},
        {"MODS", op::MODS}, {"ABSS", op::ABSS}, {"NEGS", op::NEGS}, 
        {"SHRS", op::SHRS}, {"SHLS", op::SHLS},

        {"ADDF", op::ADDF}, {"SUBF", op::SUBF}, {"MULF", op::MULF}, 
        {"DIVF", op::DIVF}, {"MODF", op::MODF}, {"ABSF", op::ABSF}, 
        {"NEGF", op::NEGF}, 

        {"VAL", op::VAL}, {"DB", op::DB}, {"DBS", op::DBS}, {"LBL", op::LBL},
    };
    
    static const std::map<op::code, std::string> op_to_name{swap_key(name_to_op)};
}
