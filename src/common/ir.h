#pragma once

#include "common/convenience.h"
#include <list>
#include <vector>
#include <iostream>
#include <type_traits>

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
        CALL, CALL_CLOSURE, PARAM, RETURN, 
        RETVAL, RETPUSH, 
        
        NEW, DELETE, 
        COPY, INDEX, OFFSET, ADDRESS, DEREFERENCE, LENGTH,
        ZERO, 
        
        NOOP
    };
}

// std::ostream& operator<<(std::ostream& os, const ir_op::code& code);
// std::string print_sequence(ir_triple* start);
