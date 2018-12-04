#include "common/type.h"

#include <cmath>
#include <utility>
#include <sstream>

#include "common/ast.h"
#include "common/symbol_table.h"

field::~field() {
    if (value) {
        delete value;
    }
}

field::field(const field& o) {
    t = o.t;
    if (o.value) {
        value = new ast;
        *value = *o.value;
    } else {
        value = nullptr;
    }
    bits = o.bits;
    name = o.name;
    bitfield = o.bitfield;
}

field::field(field&& o) {
    t = o.t;
    std::swap(value, o.value);
    bits = o.bits;
    name = o.name;
    bitfield = o.bitfield;
}

field& field::operator=(const field& o) {
    if (this != &o) {
        if (value) {
            delete value;
        }
        
        t = o.t;
        if (o.value) {
            value = new ast;
            *value = *o.value;
        } else {
            value = nullptr;
        }
        bits = o.bits;
        name = o.name;
        bitfield = o.bitfield;
    }
    return *this;
}

field& field::operator=(field&& o) {
    if (this != &o) {
        t = o.t;
        std::swap(value, o.value);
        bits = o.bits;
        name = o.name;
        bitfield = o.bitfield;
    }
    return *this;
}

type_union::type_union(const std::vector<ufield> fields, st_entry* ste, u64 def_type, ast* def_value, u64 size) 
    : fields(fields), ste(ste), def_type(def_type), def_value(def_value), size(size) {
    
}


type_union::~type_union() {
    if (def_value) {
        delete def_value;
    }
};

type_union::type_union(const type_union& o) {
    fields = o.fields;
    ste = o.ste;
    def_type = o.def_type;
    if (o.def_value) {
        def_value = new ast;
        *def_value = *o.def_value;
    } else {
        def_value = nullptr;
    }
    size = o.size;
}

type_union::type_union(type_union && o) {
    fields = o.fields;
    ste = o.ste;
    def_type = o.def_type;
    std::swap(def_value, o.def_value);
    size = o.size;
}

type_union & type_union::operator=(const type_union& o) {
    if (this != &o) {
        if (def_value) {
            delete def_value;
        }
        fields = o.fields;
        ste = o.ste;
        def_type = o.def_type;
        if (o.def_value) {
            def_value = new ast;
            *def_value = *o.def_value;
        } else {
            def_value = nullptr;
        }
        size = o.size;
    } 
    return *this;
}

type_union & type_union::operator=(type_union && o) {
    if (this != &o) {
        fields = o.fields;
        ste = o.ste;
        def_type = o.def_type;
        std::swap(def_value, o.def_value);
        size = o.size;
    } 
    return *this;
}

type::type(ettype ttype, type_id id, type_flags flags) : tt(ttype), id(id), flags(flags) {
    switch(ttype) {
        case ettype::PRIMITIVE: 
            t = type_primitive{id};
            break;
        case ettype::POINTER:
            t = type_pointer{};
            break;
        case ettype::FUNCTION: 
            t = type_function{};
            break;
        case ettype::STRUCT: 
            t = type_struct{};
            break;
        case ettype::UNION: 
            t = type_union{};
            break;
        case ettype::ENUM: 
            t = type_enum{};
            break;
        case ettype::COMBINATION: 
            t = type_combination{};
            break;
        case ettype::PSTRUCT: 
            t = type_pstruct{};
            break;
        case ettype::PFUNCTION: 
            t = type_pfunction{};
            break;
        default:
            break;
    }
}
type::type(ettype ttype, type_id id, type_flags flags, type_variant t) : tt(ttype), id(id), flags(flags), t(t) {}


type::type(type_primitive t) : tt(ettype::PRIMITIVE), t(t) {}
type::type(type_pointer t) : tt(ettype::POINTER), t(t) {}
type::type(type_pstruct t) : tt(ettype::PSTRUCT), t(t) {}
type::type(type_struct t) : tt(ettype::STRUCT), t(t) {}
type::type(type_union t) : tt(ettype::UNION), t(t) {}
type::type(type_enum t) : tt(ettype::ENUM), t(t) {}
type::type(type_combination t) : tt(ettype::COMBINATION), t(t) {}
type::type(type_function t) : tt(ettype::FUNCTION), t(t) {}
type::type(type_pfunction t) : tt(ettype::PFUNCTION), t(t) {}

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
        case ettype::PSTRUCT: { // TODO Not packed like this
            type_pstruct& ps = as_pstruct();
            if (ps.size) {
                return ps.size;
            } else {
                bool prev_was_bitfield = false;
                u64 bfsum = 0;
                for (pfield f : ps.fields) {
                    if (f.bitfield) {
                        bfsum += f.bits;
                        if (bfsum > 8) {
                            int bytes = std::floor(bfsum / 8);
                            bfsum -= bytes * 8;
                            ps.size += bytes;
                        }
                    } else {
                        if (prev_was_bitfield) {
                            ps.size++;
                            bfsum = 0;
                        }
                        ps.size += f.t->get_size();
                    }
                }
                return ps.size;
            }
        }
        case ettype::STRUCT: { 
            type_struct& s = as_struct();
            type_pstruct* ps = s.pure;
            if (ps->size) {
                return ps->size;
            } else {
                return type{type_pstruct{*ps}}.get_size();
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
                u64 size = e.ste->as_type().st->get_size();
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
        case ettype::COMBINATION:
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
        case ettype::COMBINATION: [[fallthrough]];
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
    
    if (is_primitive(etype_ids::NOTHING)) {
        return true;
    }
    
    if (is_primitive(etype_ids::NNULL) && is_ptr_type(o->tt)) { // Casting null to a pointer type
        return true;
    }
    
    if (o->tt == ettype::PRIMITIVE) {
        type_primitive& op = o->as_primitive();
        if (op.t == etype_ids::LET) {
            return true; // Can always cast to let
        }
        if (op.t == etype_ids::FUN && is_function()) {
            return true; // Can cast functions to fun
        }
    }
    
    //if (tt == ettype::PSTRUCT && o->tt == ettype::STRUCT && o->as_struct().pure == &as_pstruct()) {
    //    return true;
    //}
    
    if (tt == ettype::STRUCT || tt == ettype::UNION || tt == ettype::ENUM) { // Never allow POD casting
        return false;
    }
    
    // Cast functions to pure functions
    if (tt == ettype::FUNCTION && o->tt == ettype::PFUNCTION && as_function().pure == &o->as_pfunction()) {
        return true;
    }
    
    if (tt == ettype::PRIMITIVE && o->tt == ettype::PRIMITIVE) {
        using namespace etype_ids;
        type_primitive& tp = as_primitive(), op = o->as_primitive();
        // Allow casting from integers to larger integers
        if (is_integer_type(tp.t) && is_integer_type(op.t) && get_size() <= o->get_size()) { 
            return true;
        }
        if (is_real_type(tp.t) && op.t == DOUBLE) { // Allow casting from reals to doubles
            return true;
        }
    }
    
    if (tt == ettype::COMBINATION && o->tt == ettype::COMBINATION) {
        if (as_combination().types.size() != o->as_combination().types.size()) {
            return false;
        } else {
            auto& self_types = as_combination().types, o_types = o->as_combination().types;
            for (u64 i = 0; i < self_types.size(); ++i) {
                if (!self_types[i]->can_weak_cast(o_types[i])) {
                    return false;
                }
            }
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
        
        // Cast to and from sig
        if ((is_integer_type(tp.t) && op.t == etype_ids::SIG) || 
            (is_integer_type(op.t) && tp.t == etype_ids::SIG)) {
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

bool type::is_weak_equalish(type* o) {
    return can_weak_cast(o) && o->can_weak_cast(this);
}

bool type::is_equalish(type* o) {
    return can_cast(o) && o->can_cast(this);
}

type* type::weak_cast_target(type* a, type* b) {
    if (a->can_weak_cast(b)) {
        return a;
    } else if (b->can_weak_cast(a)) {
        return b;
    } else {
        return nullptr;
    }
}

type* type::weak_cast_result(type* a, type* b) {
    type* target = weak_cast_target(a, b);
    if (target == a) {
        return b;
    } else if (target == b) {
        return a;
    } else {
        return nullptr;
    }
}

type* type::cast_target(type* a, type* b) {
    if (a->can_cast(b)) {
        return a;
    } else if (b->can_cast(a)) {
        return b;
    } else {
        return nullptr;
    }
}

type* type::cast_result(type* a, type* b) {
    type* target = cast_target(a, b);
    if (target == a) {
        return b;
    } else if (target == b) {
        return a;
    } else {
        return nullptr;
    }
}

type* type::primitive() {
    return new type{ettype::PRIMITIVE, etype_ids::LET, 0, type_primitive{}};
}

type* type::pointer() {
    return new type{ettype::POINTER, etype_ids::LET, 0, type_pointer{}};
}

type* type::pstruct() {
    return new type{ettype::PSTRUCT, etype_ids::LET, 0, type_pstruct{}};
}

type* type::_struct() {
    return new type{ettype::STRUCT, etype_ids::LET, 0, type_struct{nullptr, {}}};
}

type* type::_union() {
    return new type{ettype::UNION, etype_ids::LET, 0, type_union{{}, nullptr}};
}

type* type::_enum() {
    return new type{ettype::ENUM, etype_ids::LET, 0, type_enum{}};
}

type* type::combination() {
    return new type{ettype::COMBINATION, etype_ids::LET, 0, type_combination{}};
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

type_pstruct&   type::as_pstruct() {
    return std::get<type_pstruct>(t);
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

type_combination& type::as_combination() {
    return std::get<type_combination>(t);
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

bool type::is_numeric() {
    return tt == ettype::PRIMITIVE && is_number_type(as_primitive().t);
}

bool type::is_integer() {
    return tt == ettype::PRIMITIVE && is_integer_type(as_primitive().t);
}

bool type::is_real() {
    return tt == ettype::PRIMITIVE && is_real_type(as_primitive().t);
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

bool type::is_struct(bool pure) {
    return tt == (pure ? ettype::PSTRUCT : ettype::STRUCT);
}

bool type::is_union() {
    return tt == ettype::UNION;
}

bool type::is_enum() {
    return tt == ettype::ENUM;
}

bool type::is_combination() {
    return tt == ettype::COMBINATION;
}

bool type::is_function(bool pure) {
    return tt == (pure ? ettype::PFUNCTION : ettype::FUNCTION);
}

std::string type::print(bool simple) {
    std::stringstream ss{};
    bool switch_signed{false};
    switch (tt) {
        case ettype::PRIMITIVE:
            switch (as_primitive().t) {
                case etype_ids::BYTE: ss << "byte"; break;
                case etype_ids::SHORT: ss << "short"; switch_signed = true; break;
                case etype_ids::INT: ss << "int"; switch_signed = true; break;
                case etype_ids::LONG: ss << "long"; switch_signed = true; break;
                case etype_ids::SIG: ss << "sig"; break;
                case etype_ids::FLOAT: ss << "float"; break;
                case etype_ids::DOUBLE: ss << "double"; break;
                case etype_ids::BOOL: ss << "bool"; break;
                case etype_ids::CHAR: ss << "char"; break;
                case etype_ids::STRING: ss << "string"; break;
                case etype_ids::VOID: ss << "void"; break;
                case etype_ids::FUN: ss << "fun"; break;
                case etype_ids::LET: ss << "let"; break;
                case etype_ids::NNULL: ss << "null"; break;
                case etype_ids::NOTHING: ss << "---"; break;
            }
            break;
        case ettype::POINTER: {
            type_pointer& ptr = as_pointer();
            ss << ptr.t->print(true);
            switch (ptr.ptr_t) {
                case eptr_type::NAKED: ss << "*"; break;
                case eptr_type::UNIQUE: ss << "*!"; break;
                case eptr_type::SHARED: ss << "*+"; break;
                case eptr_type::WEAK: ss << "*?"; break;
                case eptr_type::ARRAY: 
                    ss << "[";
                    if (ptr.size) {
                        ss << ptr.size;
                    }
                    ss << "]";
                    break;
            }
            break;
        }
        case ettype::PSTRUCT: {
            type_pstruct& psct = as_pstruct();
            ss << "struct<";
            for (auto& field : psct.fields) {
                ss << field.t->print(true);
                if (field.bitfield) {
                    ss << ": " << field.bits;
                }
                if (&field != &psct.fields.back()) {
                    ss << ", ";
                }
            }
            ss << ">";
            break;
        }
        case ettype::STRUCT: {
            type_struct& sct = as_struct();
            ss << "struct";
            if (sct.ste->name.length()) {
                ss << " " << sct.ste->name;
            }
            if (!simple && sct.ste->as_type().defined) {
                ss << " {\n";
                for (auto& field : sct.fields) {
                    ss << "\t" << field.t->print(true);
                    ss << " " << field.name;
                    if (field.bitfield) {
                        ss << ": " << field.bits;
                    }
                    if (field.value) {
                        ss << " (defaulted)";
                    }
                    ss << "\n";
                }
                ss << "}";
            }
            break;
        }
        case ettype::UNION: {
            type_union& unn = as_union();
            ss << "union";
            if (unn.ste->name.length()) {
                ss << " " << unn.ste->name;
            }
            if (!simple && unn.ste->as_type().defined) {
                ss << " [" << unn.def_type << "]";
                ss << " {\n";
                for (auto& field : unn.fields) {
                    ss << "\t" << field.t->print(true);
                    ss << " " << field.name << "\n";
                }
                ss << "}";
            }
            break;
        }
        case ettype::ENUM: {
            type_enum& enm = as_enum();
            ss << "enum " << enm.ste->name;
            break;
        }
        case ettype::FUNCTION: {
            type_function& fun = as_function();
            ss << fun.rets->print();
            if (fun.ste->name.length()) {
                ss << " " << fun.ste->name;
            }
            ss << "(";
            for (auto& param : fun.params) {
                ss << param.t->print(true);
                if (param.name.length()) {
                    ss << " " << param.name;
                }
                if (param.flags & eparam_flags::SPREAD) {
                    ss << "...";
                }
                if (param.flags & eparam_flags::DEFAULTABLE) {
                    ss << "=";
                }
                if (&param != &fun.params.back()) {
                    ss << ", ";
                }
            }
            ss << ")";
            break;
        }
        case ettype::COMBINATION: {
            type_combination& com = as_combination();
            for (type*& t : com.types) {
                ss << t->print(true);
                if (&t != &com.types.back()) {
                    ss << ":";
                }
            }
            break;
        }
        case ettype::PFUNCTION: {
            type_pfunction& pfun = as_pfunction();
            ss << "fun<" << pfun.rets->print() << "(";
            for (auto& param : pfun.params) {
                ss << param.t->print(true);
                if (param.flags & eparam_flags::SPREAD) {
                    ss << "...";
                }
                if (param.flags & eparam_flags::DEFAULTABLE) {
                    ss << "=";
                }
                if (&param != &pfun.params.back()) {
                    ss << ", ";
                }
            }
            ss << ")>";
            break;
        }
    }
    
    if ((bool)(flags & etype_flags::SIGNED) != switch_signed) {
        if (switch_signed) {
            ss << " unsigned";
        } else {
            ss << " signed";
        }
    }
    if (flags & etype_flags::CONST) {
        ss << " const";
    }
    if (flags & etype_flags::VOLATILE) {
        ss << " volatile";
    }
    
    return ss.str();
}
