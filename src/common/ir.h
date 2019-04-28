#pragma once

#include "common/convenience.h"
#include <list>
#include <vector>
#include <iostream>

struct st_entry;
struct ast;

namespace ir_op {
    enum code {
        ADD, SUBTRACT, MULTIPLY, DIVIDE,
        POWER, MODULO, 
        INCREMENT, DECREMENT, 
        NEGATE, SHIFT_LEFT, SHIFT_RIGHT,
        ROTATE_LEFT, ROTATE_RIGHT,
        AND, OR, XOR, NOT,
        
        LESS, LESS_EQUALS, GREATER, GREATER_EQUALS,
        EQUALS, NOT_EQUALS, BIT_SET, BIT_NOT_SET,
        
        JUMP, IF_ZERO, IF_NOT_ZERO,
        SYMBOL, VALUE, TEMP, 
        CALL, PARAM, RETURN, RETVAL, 
        
        COPY, INDEX, OFFSET, ADDRESS, DEREFERENCE, LENGTH,
        
        NOOP
    };
}

struct ir_triple {
    struct ir_triple_param {
        union {
            ast* value;
            st_entry* iden;
            ir_triple* triple;
            u64 immediate;
        };
        enum {LITERAL, IDEN, TRIPLE, IMMEDIATE} type{LITERAL};
        
        ir_triple_param(ast* node);
        ir_triple_param(st_entry* entry);
        ir_triple_param(ir_triple* triple);
        ir_triple_param(u64 immediate);
    };
    
    ir_op::code op{ir_op::COPY};
    ir_triple_param param1{(ast*) nullptr};
    ir_triple_param param2{(ast*) nullptr};
    
    ir_triple* next{nullptr};
    ir_triple* cond{nullptr};
    
    std::string print();
    std::string print(const std::map<ir_triple*, u64>& triples);
};

struct ir_triple_range {
    ir_triple* start;
    ir_triple* end;
    
    void append(ir_triple* triple);
    void prepend(ir_triple* triple);
};

struct block {
    block(ir_triple_range begin);
    
    void add(ir_triple_range begin);
    void add(ir_triple* triple);
    void add_end(ir_triple_range end);
    void add_end(ir_triple* triple);
    void finish();
    
    ir_triple_range start;
    ir_triple_range end{nullptr, nullptr};
};

std::ostream& operator<<(std::ostream& os, const ir_op::code& code);
std::string print_sequence(ir_triple* start);
