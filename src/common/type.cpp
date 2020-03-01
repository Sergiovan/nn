#include "common/type.h"

bool type::is_primitive() {
    return tt == type_type::PRIMITIVE;
}

bool type::is_primitive(primitive_type tt) {
    return is_primitive() && primitive.tt == tt;
}

bool type::is_pointer() {
    return tt == type_type::POINTER;
}

bool type::is_pointer(pointer_type tt) {
    return is_pointer() && pointer.tt == tt;
}

bool type::is_array() {
    return tt == type_type::ARRAY;
}

bool type::is_array(bool sized) {
    return is_array() && array.sized == sized;
}

bool type::is_compound() {
    return tt == type_type::COMPOUND;
}

bool type::is_supercompound() {
    switch (tt) {
        case type_type::STRUCT: [[fallthrough]];
        case type_type::UNION: [[fallthrough]];
        case type_type::ENUM: [[fallthrough]];
        case type_type::TUPLE:
            return true;
        default:
            return false;
    }
}

bool type::is_struct() {
    return tt == type_type::STRUCT;
}

bool type::is_union() {
    return tt == type_type::UNION;
}

bool type::is_enum() {
    return tt == type_type::ENUM;
}

bool type::is_enum(primitive_type tt) {
    return is_enum() && scompound.comp->primitive.tt == tt;
}

bool type::is_tuple() {
    return tt == type_type::TUPLE;
}

bool type::is_function() {
    return tt == type_type::FUNCTION;
}

bool type::is_superfunction() {
    return tt == type_type::SUPERFUNCTION;
}

bool type::is_supertype() {
    switch (tt) {
        case type_type::STRUCT: [[fallthrough]];
        case type_type::UNION: [[fallthrough]];
        case type_type::ENUM: [[fallthrough]];
        case type_type::TUPLE: [[fallthrough]];
        case type_type::SUPERFUNCTION:
            return true;
        default:
            return false;
    }
}

bool type::is_special() {
    return tt == type_type::SPECIAL;
}

bool type::is_special(special_type tt) {
    return is_special() && special.tt == tt;
}

type* type::get_underlying() {
    switch (tt) {
        case type_type::STRUCT: [[fallthrough]];
        case type_type::UNION: [[fallthrough]];
        case type_type::ENUM: [[fallthrough]];
        case type_type::TUPLE: 
            return scompound.comp;
        case type_type::SUPERFUNCTION:
            return sfunction.function;
        default:
            return nullptr;
    }
}

type::~type() {
    switch (tt) {
        case type_type::COMPOUND:
            compound.~type_compound();
            return;
        case type_type::FUNCTION:
            function.~type_function();
            return;
        case type_type::SUPERFUNCTION:
            sfunction.~type_superfunction();
            return;
        default:
            return;
    }
}
