#include "common/type.h"
#include <cmath>
#include <utility>

#include "common/ast.h"

field::~field() {
    if (value) {
        delete value;
    }
}

type_union::~type_union() {
    if (def_value) {
        delete def_value;
    }
};

u64 type::get_size() {
    switch (tt) {
        case ettype::PRIMITIVE:
            switch (as_primitive().t) {
                case etype_ids::BYTE: return 1;
                case etype_ids::SHORT: return 2;
                case etype_ids::INT: return 4;
                case etype_ids::LONG: return 8;
                case etype_ids::SIG: return 8;
                case etype_ids::FLOAT: return 4;
                case etype_ids::DOUBLE: return 8;
                case etype_ids::BOOL: return 1;
                case etype_ids::CHAR: return 4;
                case etype_ids::STRING: return 16;
                case etype_ids::VOID: [[fallthrough]];
                case etype_ids::FUN: [[fallthrough]];
                case etype_ids::LET: [[fallthrough]];
                case etype_ids::NNULL: [[fallthrough]];
                case etype_ids::NOTHING: [[fallthrough]];
                default:
                    return 0;
            }
        case ettype::POINTER: {
            type_pointer& p = as_pointer();
            if (p.ptr_t == eptr_type::ARRAY && p.size) {
                return p.size * p.t->get_size();
            } else {
                return 8;
            }
        }
        case ettype::STRUCT: { // TODO Not packed like this
            type_struct& s = as_struct();
            if (s.size) {
                return s.size;
            } else {
                bool prev_was_bitfield = false;
                u64 bfsum = 0;
                for (field f : s.fields) {
                    if (f.bitfield) {
                        bfsum += f.bits;
                        if (bfsum > 8) {
                            int bytes = std::floor(bfsum / 8);
                            bfsum -= bytes * 8;
                            s.size += bytes;
                        }
                    } else {
                        if (prev_was_bitfield) {
                            s.size++;
                            bfsum = 0;
                        }
                        s.size += f.t->get_size();
                    }
                }
                return s.size;
            }
        }
        case ettype::UNION: {
            type_union& u = as_union();
            if (u.size) {
                return u.size;
            } else {
                u64 max = 0;
                for (ufield f : u.fields) {
                    u64 fsize = f.t->get_size();
                    if (fsize > max) {
                        max = fsize;
                    }
                }
                u.size = max;
                return max;
            }
        }
        case ettype::ENUM: {
            type_enum& e = as_enum();
            if (e.size) {
                return e.size;
            } else {
                u64 size = e.names.size();
                if (size >> 32) {
                    e.size = 8;
                } else if (size >> 16) {
                    e.size = 4;
                } else if (size >> 8) {
                    e.size = 2;
                } else {
                    e.size = 1;
                }
                return e.size;
            }
        }
        case ettype::FUNCTION:
            return 8;
        case ettype::PSTRUCT: [[fallthrough]];
        case ettype::PFUNCTION: [[fallthrough]];
        default:
            return 0;
    }
}

bool type::can_boolean() {
    switch (tt) {
        case ettype::PRIMITIVE: [[fallthrough]];
        case ettype::FUNCTION: [[fallthrough]];
        case ettype::POINTER: return true;
        case ettype::STRUCT: [[fallthrough]];
        case ettype::UNION: [[fallthrough]];
        case ettype::ENUM: [[fallthrough]];
        case ettype::PSTRUCT: [[fallthrough]];
        case ettype::PFUNCTION: [[fallthrough]];
        default:
            return false;
    }
}

bool type::can_weak_cast(type* o) {
    if (o == this) { // Always allow casting to itself
        return true;
    }
    
    if ((flags & etype_flags::CONST) && !(o->flags & etype_flags::CONST)) { // No casting const away
        return false;
    }
    
    if (o->tt == ettype::PRIMITIVE) {
        type_primitive& op = o->as_primitive();
        if (op.t == etype_ids::LET) {
            return true; // Can always cast to let
        }
        if (op.t == etype_ids::FUN && is_function()) {
            return true; // Can cast functions to fun
        }
        if (op.t == etype_ids::NNULL && is_ptr_type(tt)) { // Casting null to a pointer type
            return true;
        }
        if (op.t ==  etype_ids::NOTHING) { // Casting Nothing to anything
            return true;
        }
    }
    
    if (tt == ettype::STRUCT || tt == ettype::UNION || tt == ettype::ENUM) { // Never allow POD casting
        return false;
    }
    
    if (tt == ettype::PRIMITIVE && o->tt == ettype::PRIMITIVE) {
        using namespace etype_ids;
        type_primitive& tp = as_primitive(), op = o->as_primitive();
        // Allow casting from integers to larger integers
        if (is_integer_type(tp.t) && is_integer_type(op.t) && get_size() < o->get_size()) { 
            return true;
        }
        if (is_real_type(tp.t) && op.t == DOUBLE) { // Allow casting from reals to doubles
            return true;
        }
    }
    
    return false;
}

bool type::can_cast(type* o) {
    if (can_weak_cast(o)) { // Weak cast can always be strong cast
        return true;
    }
    
    if ((flags & etype_flags::CONST) && !(o->flags & etype_flags::CONST)) { // NEVER EVER CAST CONST AWAY
        return false;
    }
    
    if (o->tt == ettype::PRIMITIVE && o->as_primitive().t == etype_ids::BOOL) { // Cast booleanables into booleans
        return can_boolean();
    }
    
    if (tt == ettype::PRIMITIVE && o->tt == ettype::PRIMITIVE) {
        type_primitive& tp = as_primitive(), op = o->as_primitive(); 
        if (is_number_type(tp.t) && is_number_type(op.t)) { // Cast any number to any other number
            return true;
        }
        
        if (tp.t == etype_ids::BOOL && is_number_type(op.t)) { // Cast booleans to numbers
            return true;
        }
    }
    
    if (is_primitive(etype_ids::LONG) && is_ptr_type(o->tt)) { // Cast long (64-bit number) to pointers
        return true;
    }
    
    if (is_ptr_type(tt) && is_ptr_type(o->tt)) {
        if (o->tt == ettype::POINTER) {
            type_pointer& op = o->as_pointer();
            if (op.ptr_t == eptr_type::NAKED && 
                op.t->tt == ettype::PRIMITIVE && 
                op.t->as_primitive().t == etype_ids::BYTE) { // Cast pointers to byte*
                return true;
            }
        }
        if (tt == ettype::POINTER) {
            type_pointer& tp = as_pointer();
            if (tp.ptr_t == eptr_type::NAKED && 
                tp.t->tt == ettype::PRIMITIVE && 
                tp.t->as_primitive().t == etype_ids::BYTE) { // Cast byte* to pointer
                return true;
            }
        }
    }
    
    if (tt == ettype::POINTER && o->tt == ettype::POINTER) { // Get weak pointers
        return as_pointer().ptr_t == eptr_type::SHARED && o->as_pointer().ptr_t == eptr_type::WEAK;
    }
    
    switch (tt) { // Allow nothing else
        case ettype::PRIMITIVE: [[fallthrough]];
        case ettype::POINTER: [[fallthrough]];
        case ettype::STRUCT: [[fallthrough]];
        case ettype::UNION: [[fallthrough]];
        case ettype::ENUM: [[fallthrough]];
        case ettype::FUNCTION: [[fallthrough]];
        case ettype::PSTRUCT: [[fallthrough]];
        case ettype::PFUNCTION: [[fallthrough]];
        default:
            return false;
    }
}

type* type::primitive() {
    return new type{ettype::PRIMITIVE, etype_ids::LET, 0, type_primitive{}};
}

type* type::pointer() {
    return new type{ettype::POINTER, etype_ids::LET, 0, type_pointer{}};
}

type* type::_struct() {
    return new type{ettype::STRUCT, etype_ids::LET, 0, type_struct{{}, nullptr}};
}

type* type::_union() {
    return new type{ettype::UNION, etype_ids::LET, 0, type_union{{}, nullptr}};
}

type* type::_enum() {
    return new type{ettype::ENUM, etype_ids::LET, 0, type_enum{}};
}

type* type::function() {
    return new type{ettype::FUNCTION, etype_ids::LET, 0, type_function{}};
}

type* type::pfunction() {
    return new type{ettype::PFUNCTION, etype_ids::LET, 0, type_pfunction{}};
}

type_primitive& type::as_primitive() {
    return std::get<type_primitive>(t);
}

type_pointer&   type::as_pointer() {
    return std::get<type_pointer>(t);
}

type_struct&    type::as_struct() {
    return std::get<type_struct>(t);
}

type_union&     type::as_union() {
    return std::get<type_union>(t);
}

type_enum&      type::as_enum() {
    return std::get<type_enum>(t);
}

type_function&  type::as_function() {
    return std::get<type_function>(t);
}

type_pfunction& type::as_pfunction() {
    return std::get<type_pfunction>(t);
}

bool type::is_primitive(int type) {
    return tt == ettype::PRIMITIVE && (type < 0 || as_primitive().t == type);
}

bool type::is_let() {
    return is_primitive(etype_ids::LET);
}

bool type::is_fun() {
    return is_primitive(etype_ids::FUN);
}

bool type::is_pointer() {
    return tt == ettype::POINTER;
}

bool type::is_pointer(eptr_type type) {
    return is_pointer() && as_pointer().ptr_t == type;
}

bool type::is_struct() {
    return tt == ettype::STRUCT;
}

bool type::is_union() {
    return tt == ettype::UNION;
}

bool type::is_enum() {
    return tt == ettype::ENUM;
}

bool type::is_function(bool pure) {
    return tt == (pure ? ettype::PFUNCTION : ettype::FUNCTION);
}
