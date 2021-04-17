#include "common/type.h"

#include <sstream>
#include <iomanip>
#include <cmath>

#include "common/util.h"
#include "common/type_table.h"

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

bool type::is_generic() {
    return is_special() && (special.tt == special_type::GENERIC || special.tt == special_type::GENERIC_UNKNOWN);
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

bool type::set_size() {
    ASSERT(!sized, "Type already had a size");
    switch (tt) {
        case type_type::PRIMITIVE:
            switch (primitive.tt) {
                case primitive_type::SIGNED: [[fallthrough]];
                case primitive_type::BOOLEAN: [[fallthrough]];
                case primitive_type::UNSIGNED: [[fallthrough]];
                case primitive_type::FLOATING: [[fallthrough]];
                case primitive_type::CHARACTER: [[fallthrough]];
                case primitive_type::ERROR: [[fallthrough]];
                case primitive_type::TYPE: [[fallthrough]];
                case primitive_type::ANY: 
                    size = (primitive.bits + 7) / 8;
                    sized = 1;
                    return true;
                case primitive_type::VOID:
                    size = 0;
                    sized = 1;
                    return true;
                default:
                    ASSERT(false, "Invalid primitive type");
                    return false;
            }
        case type_type::POINTER:
            size = 8;
            sized = 1;
            return true;
        case type_type::ARRAY:
            if (array.sized) {
                if (!array.at->sized && !array.at->set_size()) {
                    return false;
                }
                size = 8 + array.size * array.at->size; // TODO ???
            } else {
                size = 16; // Size + data pointer
            }
            sized = 1;
            return true;
        case type_type::COMPOUND: {
            u64 buff_size{0};
            for (auto member : compound.members) {
                u64 msize {0};
                if (member.reference) {
                    msize = 8;
                } else {
                    if (!member.t->sized && !member.t->set_size()) {
                        return false;
                    } else {
                        msize = member.t->size;
                    }
                }
                buff_size += align(buff_size, msize) + msize; // Ignore bits for now...
            }
            size = buff_size;
            sized = 1;
            return true;
        }
        case type_type::STRUCT: [[fallthrough]];
        case type_type::TUPLE:
            if (!scompound.comp->sized && !scompound.comp->set_size()) {
                return false;
            }
            size = scompound.comp->size;
            sized = 1;
            return true;
        case type_type::ENUM: 
            if (!scompound.comp->sized) {
                scompound.comp->set_size();
            }
            size = scompound.comp->size;
            sized = 1;
            return true;
        case type_type::UNION:
            if (!scompound.comp->sized && !scompound.comp->set_size()) {
                return false;
            }
            sized = 1;
            // No references inside unions
            for (auto member : compound.members) {
                size = size > member.t->size ? size : member.t->size;
            }
            return true;
        case type_type::FUNCTION: [[fallthrough]];
        case type_type::SUPERFUNCTION: 
            size = 16; // TODO
            sized = 1;
            return true;
        case type_type::SPECIAL:
            size = special.tt == special_type::NULL_ ? 8 : 1; // :D
            sized = 1; // :S
            return true;
        default:
            ASSERT(false, "Invalid type type");
            return false;
    }
}

type::type(type_table& tt, u64 id, const type_primitive& primitive, bool _const, bool volat) 
: table{tt}, id{id}, tt{type_type::PRIMITIVE}, const_flag{_const}, volat_flag{volat}, 
    sized{0}, size{0}, primitive{primitive} {
    
}

type::type(type_table& tt, u64 id, const type_pointer& pointer, bool _const, bool volat) 
: table{tt}, id{id}, tt{type_type::POINTER}, const_flag{_const}, volat_flag{volat}, 
    sized{0}, size{0}, pointer{pointer} {
    
}

type::type(type_table& tt, u64 id, const type_array& array, bool _const, bool volat) 
: table{tt}, id{id}, tt{type_type::ARRAY}, const_flag{_const}, volat_flag{volat}, 
    sized{0}, size{0}, array{array} {
    
}

type::type(type_table& tt, u64 id, const type_compound& compound, bool _const, bool volat) 
: table{tt}, id{id}, tt{type_type::COMPOUND}, const_flag{_const}, volat_flag{volat}, 
    sized{0}, size{0}, compound{compound} {
    
}

type::type(type_table& tt, u64 id, type_type ttt, const type_supercompound& scompound, 
           bool _const, bool volat) 
: table{tt}, id{id}, tt{ttt}, const_flag{_const}, volat_flag{volat}, 
    sized{0}, size{0}, scompound{scompound} {
    
}

type::type(type_table& tt, u64 id, const type_function& function, bool _const, bool volat) 
: table{tt}, id{id}, tt{type_type::FUNCTION}, const_flag{_const}, volat_flag{volat}, 
    sized{0}, size{0}, function{function} {
    
}

type::type(type_table& tt, u64 id, const type_superfunction& sfunction, bool _const, bool volat) 
: table{tt}, id{id}, tt{type_type::SUPERFUNCTION}, const_flag{_const}, volat_flag{volat}, 
    sized{0}, size{0}, sfunction{sfunction} {
    
}

type::type(type_table& tt, u64 id, const type_special& special, bool _const, bool volat) 
: table{tt}, id{id}, tt{type_type::SPECIAL}, const_flag{_const}, volat_flag{volat}, 
    sized{0}, size{0}, special{special} {
    
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
: table{o.table}, id{o.id}, tt{o.tt}, const_flag{o.const_flag}, volat_flag{o.volat_flag},
  sized{o.sized}, size{o.size} {
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
            std::swap(function, o.function);
            break;
        }
        case type_type::SUPERFUNCTION: {
            std::swap(sfunction, o.sfunction);
            break;
        }
        case type_type::SPECIAL: {
            std::swap(special, o.special);
            break;
        }
    }
    o.tt = type_type::SPECIAL; // Call no destructors
}

std::string type::to_string(bool simple) {
    std::stringstream ss{};
    
    if (!simple) {
        ss << "(" << std::hex << id << ") " << std::dec;
    }
    
    if (const_flag) {
        ss << "const ";
    }
    if (volat_flag) {
        ss << "volat ";
    }
    
    switch (tt) {
        case type_type::PRIMITIVE:
            switch (primitive.tt) {
                case primitive_type::SIGNED:
                    ss << "s" << primitive.bits;
                    break;
                case primitive_type::BOOLEAN:
                    ss << "bool ";
                    [[fallthrough]];
                case primitive_type::UNSIGNED:
                    ss << "u" << primitive.bits;
                    break;
                case primitive_type::FLOATING:
                    ss << "f" << primitive.bits;
                    break;
                case primitive_type::CHARACTER:
                    ss << "c" << primitive.bits;
                    break;
                case primitive_type::ERROR:
                    ss << "e" << primitive.bits;
                    break;
                case primitive_type::TYPE:
                    ss << "type";
                    break;
                case primitive_type::ANY:
                    ss << "any";
                    break;
                case primitive_type::VOID:
                    ss << "u0";
                    break;
            }
            break;
        case type_type::POINTER:
            switch (pointer.tt) {
                case pointer_type::NAKED:
                    ss << "*" << pointer.at->to_string(true);
                    break;
                case pointer_type::UNIQUE:
                    ss << "!" << pointer.at->to_string(true);
                    break;
                case pointer_type::SHARED:
                    ss << "+" << pointer.at->to_string(true);
                    break;
                case pointer_type::WEAK:
                    ss << "?" << pointer.at->to_string(true);
                    break;
            }
            break;
        case type_type::ARRAY:
            ss << "[";
            if (array.sized) {
                ss << array.size;
            }
            ss << "]" << array.at->to_string(true);
            break;
        case type_type::COMPOUND:
            for (auto member : compound.members) {
                if (member.compiletime) {
                    ss << "let ";
                }
                if (member.reference) {
                    ss << "ref ";
                }
                ss << ": " << member.t->to_string(true);
            }
            break;
        case type_type::STRUCT: [[fallthrough]];
        case type_type::UNION: [[fallthrough]];
        case type_type::ENUM: [[fallthrough]];
        case type_type::TUPLE:
            ss << tt << " " << scompound.comp->to_string(true);
            break;
        case type_type::FUNCTION:
            ss << "fun (";
            for (auto& p : function.params) {
                if (p.compiletime) {
                    ss << "let ";
                }
                if (p.reference) {
                    ss << "ref ";
                }
                if (p.thisarg) {
                    ss << "this ";
                }
                if (p.binding) {
                    ss << "::";
                } else {
                    ss << ":";
                }
                ss << p.t->to_string(simple);
                if (p.spread) {
                    ss << "...";
                }
                ss << ", ";
            }
            ss << "-> ";
            for (auto& r : function.rets) {
                if (r.reference) {
                    ss << "ref ";
                }
                if (r.compiletime) {
                    ss << "let ";
                }
                ss << ":" << r.t->to_string(simple);
                ss << ", ";
            }
            ss << ")";
            break;
        case type_type::SUPERFUNCTION: {
            ss << "fun (";
            type_function& fn = sfunction.function->function;
            type_superfunction& sfn = sfunction;
            for (u64 i = 0; i < sfunction.params.size(); ++i) {
                auto& p = fn.params[i];
                auto& sp = sfn.params[i];
                if (p.reference) {
                    ss << "ref ";
                }
                if (p.compiletime) {
                    ss << "let ";
                }
                if (p.thisarg) {
                    ss << "this ";
                }
                ss << sp.name;
                if (p.generic) {
                    ss << "::";
                } else {
                    ss << ":";
                }
                ss << p.t->to_string(simple);
                if (p.spread) {
                    ss << "...";
                }
                if (sp.defaulted) {
                    ss << " =";
                }
                ss << ", ";
            }
            ss << "-> ";
            for (u64 i = 0; i < sfunction.rets.size(); ++i) {
                auto& r = fn.rets[i];
                auto& sr = sfn.rets[i];
                if (r.reference) {
                    ss << "ref ";
                }
                if (r.compiletime) {
                    ss << "let ";
                }
                ss << sr.name << ": " << r.t->to_string(simple);
                ss << ", ";
            }
            break;
        }
        case type_type::SPECIAL:
            switch (special.tt) {
                case special_type::INFER:
                    ss << "infer";
                    break;
                case special_type::GENERIC:
                    ss << "generic '" << id;
                    break;
                case special_type::GENERIC_UNKNOWN:
                    ss << "generic unknown";
                    break;
                case special_type::GENERIC_COMPOUND:
                    ss << "generic compound";
                    break;
                case special_type::NOTHING:
                    ss << "---";
                    break;
                case special_type::TYPELESS:
                    ss << "typeless";
                    break;
                case special_type::NONE:
                    ss << "undecided";
                    break;
                case special_type::NONE_ARRAY:
                    ss << "undecided array";
                    break;
                case special_type::NONE_STRUCT:
                    ss << "undecided struct";
                    break;
                case special_type::NONE_TUPLE:
                    ss << "undecided tuple";
                    break;
                case special_type::NONE_FUNCTION:
                    ss << "undecided function";
                    break;
                case special_type::NULL_:
                    ss << "null";
                    break;
                case special_type::ERROR_TYPE:
                    ss << "error";
                    break;
                case special_type::ERROR_COMPOUND:
                    ss << "error compound";
                    break;
            }
            break;
    }
    return ss.str();
}

std::ostream& operator<<(std::ostream& os, type_type t) {
    switch (t) {
        case type_type::PRIMITIVE:
            return os << "PRIMITIVE";
        case type_type::POINTER:
            return os << "POINTER";
        case type_type::ARRAY:
            return os << "ARRAY";
        case type_type::COMPOUND:
            return os << "COMPOUND";
        case type_type::STRUCT:
            return os << "STRUCT";
        case type_type::UNION:
            return os << "UNION";
        case type_type::ENUM:
            return os << "ENUM";
        case type_type::TUPLE:
            return os << "TUPLE";
        case type_type::FUNCTION:
            return os << "FUNCTION";
        case type_type::SUPERFUNCTION:
            return os << "SUPERFUNCTION";
        case type_type::SPECIAL:
            return os << "SPECIAL";
        default:
            return os << "INVALID TYPE";
    }
}

std::ostream& operator<<(std::ostream& os, primitive_type t) {
    switch (t) {
        case primitive_type::SIGNED:
            return os << "SIGNED";
        case primitive_type::UNSIGNED:
            return os << "UNSIGNED";
        case primitive_type::BOOLEAN:
            return os << "BOOLEAN";
        case primitive_type::FLOATING:
            return os << "FLOATING";
        case primitive_type::CHARACTER:
            return os << "CHARACTER";
        case primitive_type::ERROR:
            return os << "ERROR";
        case primitive_type::TYPE:
            return os << "TYPE";
        case primitive_type::ANY:
            return os << "ANY";
        case primitive_type::VOID:
            return os << "VOID";
        default:
            return os << "INVALID PRIMITIVE";
    }
}

std::ostream& operator<<(std::ostream& os, pointer_type t) {
    switch (t) {
        case pointer_type::NAKED:
            return os << "NAKED";
        case pointer_type::UNIQUE:
            return os << "UNIQUE";
        case pointer_type::SHARED:
            return os << "SHARED";
        case pointer_type::WEAK:
            return os << "WEAK";
        default:
            return os << "INVALID POINTER";
    }
}

std::ostream& operator<<(std::ostream& os, special_type t) {
    switch (t) {
        case special_type::INFER:
            return os << "INFER";
        case special_type::GENERIC:
            return os << "GENERIC";
        case special_type::NOTHING:
            return os << "NOTHING";
        case special_type::TYPELESS:
            return os << "TYPELESS";
        case special_type::NONE:
            return os << "NONE";
            
        case special_type::NONE_ARRAY:
            return os << "NONE_ARRAY";
        case special_type::NONE_STRUCT:
            return os << "NONE_STRUCT";
        case special_type::NONE_TUPLE:
            return os << "NONE_TUPLE";
        case special_type::NONE_FUNCTION:
            return os << "NONE_FUNCTION";
        case special_type::NULL_:
            return os << "NULL";
        case special_type::ERROR_TYPE:
            return os << "ERROR TYPE";
        default:
            return os << "INVALID SPECIAL";
    }
}
