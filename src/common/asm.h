#pragma once

#include <map>
#include "common/convenience.h"

namespace nnasm {
    namespace op {
        enum code {
            NOP, MV, CP, ZRO, SET, BRK, HLT,
            
            JMP, JZ, JNZ, JEQ, JNE, JL, JLE,
            JG, JGE, 
            
            PUSH, POP, CALL, RET,
            
            ADD, INC, SUB, DEC, MUL, DIV, MOD,
            ABS, NEG, SHR, SHL, RTR, RTL, AND,
            OR, XOR, NOT,
            
            ADDF, SUBF, MULF, DIVF, MODF, ABSF, 
            NEGF, 
            
            VAL, DB, DBS, LBL,
            
            INVALID_CODE
        };
        
        enum operand {
            OP_NONE = 0,
            OP_REG = 1, OP_VAL = 2, OP_ADDR = 3,
            
            OP_REG_ADDR, OP_ANY,
            OP_IDN, 
        };
        
        enum register_type {
            QWORD = 0 << 4,
            BYTE = 1 << 4,
            WORD = 2 << 4,
            DWORD = 3 << 4,
            FLOAT = 4 << 4,
            DOUBLE = 5 << 4,
            
            SIGNED = 1 << 7
        };
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
    
    static const dict<op::code> name_to_op {{"NOP", op::NOP}, 
        {"MV", op::MV}, {"CP", op::CP}, {"ZRO", op::ZRO}, 
        {"SET", op::SET}, {"BRK", op::BRK}, {"HLT", op::HLT}, 

        {"JMP", op::JMP}, {"JZ", op::JZ}, {"JNZ", op::JNZ}, 
        {"JEQ", op::JEQ}, {"JNE", op::JNE}, {"JL", op::JL}, 
        {"JLE", op::JLE}, {"JG", op::JG}, {"JGE", op::JGE}, 
        
        {"PUSH", op::PUSH}, {"POP", op::POP}, {"CALL", op::CALL}, 
        {"RET", op::RET}, 

        {"ADD", op::ADD}, {"INC", op::INC}, {"SUB", op::SUB}, 
        {"DEC", op::DEC}, {"MUL", op::MUL}, {"DIV", op::DIV}, 
        {"MOD", op::MOD}, {"ABS", op::ABS}, {"NEG", op::NEG}, 
        {"SHR", op::SHR}, {"SHL", op::SHL}, {"RTR", op::RTR}, 
        {"RTL", op::RTL}, {"AND", op::AND}, {"OR", op::OR}, 
        {"XOR", op::XOR}, {"NOT", op::NOT}, 

        {"VAL", op::VAL}, {"DB", op::DB}, {"DBS", op::DBS}, {"LBL", op::LBL}
    };
    
    static const std::map<op::code, std::string> op_to_name{swap_key(name_to_op)};
}
