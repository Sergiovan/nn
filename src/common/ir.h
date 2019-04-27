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
        INCREMENT, DECREMENT, 
        NEGATE, SHIFT_LEFT, SHIFT_RIGHT,
        ROTATE_LEFT, ROTATE_RIGHT,
        AND, OR, XOR, NOT,
        
        JUMP, IF_ZERO, IF_NOT_ZERO,
        IF_LESS_THAN_ZERO, IF_GREATER_THAN_ZERO,
        IF_BIT_SET, IF_BIT_NOT_SET,
        
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
    
    std::string print();
};

struct ir {
    ir();
    
    ~ir();
    ir(const ir& o) = delete;
    ir(ir&& o);
    ir& operator=(const ir& o) = delete;
    ir& operator=(ir&& o);
    
    void add(ir_triple t);
    void add(ir_triple t, u64 at);
    
    void merge_in(ir&& o);
    void merge_in(ir&& o, u64 at);
    
    void move(u64 from, u64 to);
    
    std::vector<ir_triple*> triples{}; // Owned
    ir* next{nullptr}; // Not owned
    ir* cond{nullptr}; // Not owned
    
    std::string print();
};

struct block {
    block(ir* begin);
    
    void add(ir* new_ir);
    void add_end(ir* new_end);
    void finish();
    
    ir* start{nullptr}; // Not owned
    ir* latest{nullptr}; // Not owned
    ir* end{nullptr};   // Not owned
};

std::ostream& operator<<(std::ostream& os, const ir_op::code& code);
std::string print_sequence(ir* start);
