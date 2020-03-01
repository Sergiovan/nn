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

type::type(type_table& tt, u64 id, const type_primitive& primitive, bool _const, bool volat) 
: table{tt}, id{id}, tt{type_type::PRIMITIVE}, const_flag{_const}, volat_flag{volat}, primitive{primitive} {
    
}

type::type(type_table& tt, u64 id, const type_pointer& pointer, bool _const, bool volat) 
: table{tt}, id{id}, tt{type_type::POINTER}, const_flag{_const}, volat_flag{volat}, pointer{pointer} {
    
}

type::type(type_table& tt, u64 id, const type_array& array, bool _const, bool volat) 
: table{tt}, id{id}, tt{type_type::ARRAY}, const_flag{_const}, volat_flag{volat}, array{array} {
    
}

type::type(type_table& tt, u64 id, const type_compound& compound, bool _const, bool volat) 
: table{tt}, id{id}, tt{type_type::COMPOUND}, const_flag{_const}, volat_flag{volat}, compound{compound} {
    
}

type::type(type_table& tt, u64 id, type_type ttt, const type_supercompound& scompound, bool _const, bool volat) 
: table{tt}, id{id}, tt{ttt}, const_flag{_const}, volat_flag{volat}, scompound{scompound} {
    
}

type::type(type_table& tt, u64 id, const type_function& function, bool _const, bool volat) 
: table{tt}, id{id}, tt{type_type::FUNCTION}, const_flag{_const}, volat_flag{volat}, function{function} {
    
}

type::type(type_table& tt, u64 id, const type_superfunction& sfunction, bool _const, bool volat) 
: table{tt}, id{id}, tt{type_type::SUPERFUNCTION}, const_flag{_const}, volat_flag{volat}, sfunction{sfunction} {
    
}

type::type(type_table& tt, u64 id, const type_special& special, bool _const, bool volat) 
: table{tt}, id{id}, tt{type_type::SPECIAL}, const_flag{_const}, volat_flag{volat}, special{special} {
    
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

type::type(type&& o) 
: table{o.table}, id{o.id}, tt{o.tt}, const_flag{o.const_flag}, volat_flag{o.volat_flag} {
    switch (tt) {
        case type_type::PRIMITIVE: {
            std::swap(primitive, o.primitive);
            break;
        }
        case type_type::POINTER: {
            std::swap(pointer, o.pointer);
            break;
        }
        case type_type::ARRAY: {
            std::swap(array, o.array);
            break;
        }
        case type_type::COMPOUND: {
            compound = type_compound{};
            std::swap(compound, o.compound);
            break;
        }
        case type_type::STRUCT: [[fallthrough]];
        case type_type::UNION: [[fallthrough]];
        case type_type::ENUM: [[fallthrough]];
        case type_type::TUPLE: {
            std::swap(scompound, o.scompound);
            break;
        }        
        case type_type::FUNCTION: {
            function = type_function{};
            std::swap(function, o.function);
            break;
        }
        case type_type::SUPERFUNCTION: {
            sfunction = type_superfunction{};
            std::swap(sfunction, o.sfunction);
            break;
        }
        case type_type::SPECIAL: {
            std::swap(special, o.special);
            break;
        }
    }
}
