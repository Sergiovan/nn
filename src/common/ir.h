#pragma once

#include "common/convenience.h"
#include <cstring>
#include <list>
#include <vector>
#include <iostream>
#include <type_traits>

struct type;
struct st_entry;
struct ast;
struct ir_triple;

/* TODO 
    Simplify: CONCATENATE
 */
namespace ir_op {
    enum code {
        ADD, SUBTRACT, MULTIPLY, DIVIDE,
        POWER, MODULO, 
        INCREMENT, DECREMENT, 
        NEGATE, SHIFT_LEFT, SHIFT_RIGHT,
        ROTATE_LEFT, ROTATE_RIGHT,
        AND, OR, XOR, NOT,
        CONCATENATE,
        
        CAST_FTD, CAST_DTF, CAST_STU, CAST_UTS,
        CAST_UTF, CAST_STF, CAST_UTD, CAST_STD,
        CAST_FTU, CAST_FTS, CAST_DTU, CAST_DTS,
        
        LESS, LESS_EQUALS, GREATER, GREATER_EQUALS,
        EQUALS, NOT_EQUALS, BIT_SET, BIT_NOT_SET,
        
        JUMP, IF_FALSE, IF_TRUE, 
        IF_ZERO = IF_FALSE, IF_NOT_ZERO = IF_TRUE,
        SYMBOL, VALUE, TEMP, 
        CALL, CALL_CLOSURE, PARAM, RETURN, RETVAL,
        
        NEW, DELETE, 
        COPY, INDEX, OFFSET, ADDRESS, DEREFERENCE, LENGTH,
        ZERO, 
        
        NOOP
    };
}

struct ir_triple_param {
    union {
        ast* value;
        st_entry* iden;
        ir_triple* triple;
        u64 immediate;
    };
    enum {VALUE, IDEN, TRIPLE, IMMEDIATE} type {VALUE};
    
    ir_triple_param(u64 immediate);
    ir_triple_param(ast* value);
    ir_triple_param(st_entry* iden);
    ir_triple_param(ir_triple* triple);
    ir_triple_param(std::nullptr_t);
    
    void set(u64 immediate);
    void set(ast* value);
    void set(st_entry* iden);
    void set(ir_triple* triple);
    void set(std::nullptr_t);
    
    operator ast*() const;
    operator st_entry*() const;
    operator ir_triple*() const;
    operator u64() const;
    
    std::string print();
};

struct ir_triple {
    ir_op::code op{ir_op::NOOP};
    ir_triple_param op1{nullptr};
    ir_triple_param op2{nullptr};
    
    ir_triple* next{nullptr};
    ir_triple* cond{nullptr};
    
    type* res_type{nullptr};
    
    std::string label{};
    u64 id{0};
    
    std::string print();
};

// std::ostream& operator<<(std::ostream& os, const ir_op::code& code);
// std::string print_sequence(ir_triple* start);
