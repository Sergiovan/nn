#include "common/type_table.h"

#include <cstring>

type_table::type_table() {
    U0 = add_primitive({primitive_type::VOID, 0}, false, false);
    U1 = add_primitive({primitive_type::BOOLEAN, 1}, false, false);
    U8 = add_primitive({primitive_type::UNSIGNED, 8}, false, false);
    U16 = add_primitive({primitive_type::UNSIGNED, 16}, false, false);
    U32 = add_primitive({primitive_type::UNSIGNED, 32}, false, false);
    U64 = add_primitive({primitive_type::UNSIGNED, 64}, false, false);
    S8 = add_primitive({primitive_type::SIGNED, 8}, false, false);
    S16 = add_primitive({primitive_type::SIGNED, 16}, false, false);
    S32 = add_primitive({primitive_type::SIGNED, 32}, false, false);
    S64 = add_primitive({primitive_type::SIGNED, 64}, false, false);
    F32 = add_primitive({primitive_type::FLOATING, 32}, false, false);
    F64 = add_primitive({primitive_type::FLOATING, 64}, false, false);
    C8 = add_primitive({primitive_type::CHARACTER, 8}, false, false);
    C16 = add_primitive({primitive_type::CHARACTER, 16}, false, false);
    C32 = add_primitive({primitive_type::CHARACTER, 32}, false, false);
    E64 = add_primitive({primitive_type::ERROR, 64}, false, false);
    TYPE = add_primitive({primitive_type::TYPE, 64}, false, false);
    ANY = add_primitive({primitive_type::ANY, 64}, false, false);
    INFER = add_special({special_type::INFER}, false, false);
    NOTHING = add_special({special_type::NOTHING}, false, false);
    TYPELESS = add_special({special_type::TYPELESS}, false, false);
    NONE = add_special({special_type::NONE}, false, false);
    NONE_ARRAY = add_special({special_type::NONE_ARRAY}, false, false);
    NONE_STRUCT = add_special({special_type::NONE_STRUCT}, false, false);
    NONE_TUPLE = add_special({special_type::NONE_TUPLE}, false, false);
    NONE_FUNCTION = add_special({special_type::NONE_FUNCTION}, false, false);
    NULL_ = add_special({special_type::NULL_}, false, false);
    GENERIC_UNKNOWN = add_special({special_type::GENERIC_UNKNOWN}, false, false);
    GENERIC_COMPOUND = add_special({special_type::GENERIC_COMPOUND}, false, false);
    ERROR_TYPE = add_special({special_type::ERROR_TYPE}, false, false);
    ERROR_COMPOUND = add_special({special_type::ERROR_COMPOUND}, false, false);
}

type* type_table::add_primitive(const type_primitive& p, const bool _const, const bool volat) {
    type t {*this, types.size(), p, _const, volat};
    t.set_size();
    return get_or_add(&t);
}

type* type_table::add_pointer(const type_pointer& p, const bool _const, const bool volat) {
    type t {*this, types.size(), p, _const, volat};
    t.set_size();
    return get_or_add(&t);
}

type* type_table::add_array(const type_array& a, const bool _const, const bool volat) {
    type t {*this, types.size(), a, _const, volat};
    t.set_size();
    return get_or_add(&t);
}

type* type_table::add_compound(const type_compound& c, const bool _const, const bool volat) {
    type t {*this, types.size(), c, _const, volat};
    t.set_size();
    return get_or_add(&t);
}

type* type_table::add_supercompound(const type_supercompound& sc, type_type sct, const bool _const, const bool volat) {
    type t {*this, types.size(), sct, sc, _const, volat};
    t.set_size();
    return get_or_add(&t);
}

type* type_table::add_function(const type_function& f, const bool _const, const bool volat) {
    type t {*this, types.size(), f, _const, volat};
    t.set_size();
    return get_or_add(&t);
}

type* type_table::add_superfunction(const type_superfunction& sf, const bool _const, const bool volat) {
    type t {*this, types.size(), sf, _const, volat};
    t.set_size();
    return get_or_add(&t);
}

type* type_table::add_special(const type_special& s, const bool _const, const bool volat) {
    type t {*this, types.size(), s, _const, volat};
    t.set_size();
    return get_or_add(&t);
}

type* type_table::add_temp(type& t) {
    type* nt = new type{std::move(*&t)};
    nt->id = types.size();
    types.push_back(nt);
    // No mangle table
    return nt;
}

// TODO Remove duplicates and keep open spots tracked
type* type_table::update_temp(type* t) {
    switch (t->tt) {
        case type_type::PRIMITIVE: [[fallthrough]];
        case type_type::SPECIAL: {
            ASSERT(false, "Primitive and special types cannot be temporaries");
            return nullptr;
        }
        case type_type::POINTER: [[fallthrough]];
        case type_type::ARRAY: [[fallthrough]];
        case type_type::STRUCT: [[fallthrough]];
        case type_type::UNION: [[fallthrough]];
        case type_type::ENUM: [[fallthrough]];
        case type_type::TUPLE: [[fallthrough]];
        case type_type::SUPERFUNCTION: {
            mangle_table.insert({mangle(t), t});
            return t;
        }
        case type_type::COMPOUND: [[fallthrough]];
        case type_type::FUNCTION: {
            std::string mangled = mangle(t);
            if (mangle_table.count(mangled) > 0) { // Already exists...
                types[t->id] = nullptr;
                delete t;
                return mangle_table[mangled];
            } else {
                mangle_table.insert({mangled, t});
                return t;
            }
        }
    }
    
    ASSERT(false, "Unknown type type");
    return nullptr;
}

type* type_table::get(u64 id) const {
    return types[id];
}

type* type_table::operator[](u64 id) const {
    return get(id);
}

type* type_table::get(const type_primitive& p, const bool _const, const bool volat) {
    type t {*this, types.size(), p, _const, volat};
    return get(&t);
}

type* type_table::get(const type_pointer& p, const bool _const, const bool volat) {
    type t {*this, types.size(), p, _const, volat};
    return get(&t);
}

type* type_table::get(const type_array& a, const bool _const, const bool volat) {
    type t {*this, types.size(), a, _const, volat};
    return get(&t);
}

type* type_table::get(const type_compound& c, const bool _const, const bool volat) {
    type t {*this, types.size(), c, _const, volat};
    return get(&t);
}

type* type_table::get(const type_supercompound& sc, type_type sct, const bool _const, const bool volat) {
    type t {*this, types.size(), sct, sc, _const, volat};
    return get(&t);
}

type* type_table::get(const type_function& f, const bool _const, const bool volat) {
    type t {*this, types.size(), f, _const, volat};
    return get(&t);
}

type* type_table::get(const type_superfunction& sf, const bool _const, const bool volat) {
    type t {*this, types.size(), sf, _const, volat};
    return get(&t);
}

type* type_table::get(const type_special& s, const bool _const, const bool volat) {
    type t {*this, types.size(), s, _const, volat};
    return get(&t);
}

type* type_table::reflag(type* t, const bool _const, const bool volat) {
    type nt {*this, types.size(), type_primitive{}, _const, volat};
    
    switch (t->tt) {
        case type_type::PRIMITIVE: {
            nt.primitive = t->primitive;
            break;
        }
        case type_type::POINTER: {
            nt.pointer = t->pointer;
            break;
        }
        case type_type::ARRAY: {
            nt.array = t->array;
            break;
        }
        case type_type::COMPOUND: {
            nt.compound = t->compound;
            break;
        }
        case type_type::STRUCT: [[fallthrough]];
        case type_type::UNION: [[fallthrough]];
        case type_type::ENUM: [[fallthrough]];
        case type_type::TUPLE: {
            nt.scompound = t->scompound;
            break;
        }        
        case type_type::FUNCTION: {
            nt.function = t->function;
            break;
        }
        case type_type::SUPERFUNCTION: {
            nt.sfunction = t->sfunction;
            break;
        }
        case type_type::SPECIAL: {
            nt.special = t->special;
            break;
        }
    }
    
    return get_or_add(&nt);
}

type* type_table::pointer_to(type* t, pointer_type pt, bool _const, bool volat) {
    return add_pointer({pt, t}, _const, volat);
}

type* type_table::array_of(type* t, bool _const, bool volat) {
    return add_array({t, false, 0}, _const, volat);
}

type* type_table::sized_array_of(type* t, u64 size, bool _const, bool volat) {
    return add_array({t, true, size}, _const, volat);
}


// TODO This should be done with maps/tables, hmm
bool type_table::can_convert_strong(type* from, type* to) {
    // All primitives and non-types cast to boolean
    if (to == U1 && !from->is_compound() && !from->is_function() && from != TYPE) { // Keep an eye on this one
        return true;
    }
    
    // All numbers cast between themselves
    if ((to->is_primitive(primitive_type::SIGNED) || to->is_primitive(primitive_type::UNSIGNED) || to->is_primitive(primitive_type::FLOATING)) &&
        (from->is_primitive(primitive_type::SIGNED) || from->is_primitive(primitive_type::UNSIGNED) || from->is_primitive(primitive_type::FLOATING))) {
        return true;
    }
    
    // All characters cast between themselves
    if (to->is_primitive(primitive_type::CHARACTER) && from->is_primitive(primitive_type::CHARACTER)) {
        return true;
    }
    
    // Characters, errors, types and pointers all cast to unsigneds
    if (to->is_primitive(primitive_type::UNSIGNED) && (from->is_primitive(primitive_type::CHARACTER) || from == E64 || from == TYPE || from->is_pointer())) {
        return true;
    }
    
    // Unsigneds cast to characters, errors, types and pointers
    if (from->is_primitive(primitive_type::UNSIGNED) && (to->is_primitive(primitive_type::CHARACTER) || to == E64 || to == TYPE || to->is_pointer())) {
        return true;
    }
    
    // ANY casts to anything except NOTHING, INFER and U0
    if (from == ANY && to != NOTHING && to != INFER && to != U0) {
        return true;
    }
    
    // Pointers cast to naked pointers to the same type
    if (from->is_pointer() && to->is_pointer(pointer_type::NAKED) && from->pointer.at == to->pointer.at) {
        return true;
    }
    
    // Weak and shared pointers cast between themselves as long as they point to the same type
    if (((from->is_pointer(pointer_type::SHARED) && to->is_pointer(pointer_type::WEAK)) || 
        (from->is_pointer(pointer_type::WEAK) && to->is_pointer(pointer_type::SHARED))) && from->pointer.at == to->pointer.at) {
        return true;
    }
    
    // Naked pointers can always cast from and to naked pointers to U8
    if (from->is_pointer(pointer_type::NAKED) && to->is_pointer(pointer_type::NAKED) && (from->pointer.at == U8 || to->pointer.at == U8)) {
        return true;
    }
    
    // Arrays can always cast between themselves as long as they point to the same type
    if (from->is_array() && to->is_array() && from->array.at == to->array.at) {
        return true;
    }
    
    // All weak type conversions apply
    return can_convert_weak(from, to);
}

bool type_table::can_convert_weak(type* from, type* to) {
    // Everything casts to itself
    if (from == to) {
        return true;
    }
    
    // GENERIC casts to everything and from everything by default
    if (from->is_special(special_type::GENERIC) || to->is_special(special_type::GENERIC)) {
        return true; // TODO ???
    }
    
    // GENERIC_UNKNOWN casts to everything and from everything by default
    if (from == GENERIC_UNKNOWN || to == GENERIC_UNKNOWN) {
        return true;
    }
    
    // ERROR_TYPE propagates without generating extra errors
    if (from == ERROR_TYPE || to == ERROR_TYPE) {
        return true;
    }
    
    // U0 doesn't cast to anything, and you cannot cast to NOTHING
    if (from == U0 || to == U0 || to == NOTHING) {
        return false;
    }
    
    // Anything except null casts to ANY
    if (to == ANY && from != NULL_) {
        return true;
    }
    
    // Anything with a proper type casts to INFER
    if (to == INFER && from != NULL_ && from != NOTHING) {
        return true;
    }
    
    // Unsigned values upcast
    if (to->is_primitive(primitive_type::UNSIGNED) && from->is_primitive(primitive_type::UNSIGNED) && to->primitive.bits >= from->primitive.bits) {
        return true;
    }
    
    // Signed values upcast
    if (to->is_primitive(primitive_type::SIGNED) && from->is_primitive(primitive_type::SIGNED) && to->primitive.bits >= from->primitive.bits) {
        return true;
    }
    
    // Characters upcast
    if (to->is_primitive(primitive_type::CHARACTER) && from->is_primitive(primitive_type::CHARACTER) && to->primitive.bits >= from->primitive.bits) {
        return true;
    }
    
    // null casts to any pointer except weak
    if (from == NULL_ && to->is_pointer() && !to->is_pointer(pointer_type::WEAK)) {
        return true;
    }
    
    // Sized arrays cast to unsized arrays
    if (from->is_array(true) && to->is_array(false) && from->array.at == to->array.at) {
        return true;
    }
    
    return false;
}

type* type_table::propagate_generic(type* t) {
    return t->is_generic() ? GENERIC_UNKNOWN : t;
}

type* type_table::get_signed(u16 bits) {
    switch (bits) {
        case 8: return S8;
        case 16: return S16;
        case 32: return S32;
        case 64: return S64;
        default:
            return get(type_primitive{primitive_type::SIGNED, bits}, false, false);
    }
}

type* type_table::get_unsigned(u16 bits) {
    switch (bits) {
        case 8: return U8;
        case 16: return U16;
        case 32: return U32;
        case 64: return U64;
        default:
            return get(type_primitive{primitive_type::UNSIGNED, bits}, false, false);
    }
}

type* type_table::add(type* t) {
    type* nt = new type{std::move(*t)};
    nt->id = types.size();
    types.push_back(nt);
    mangle_table.insert({mangle(nt), nt});
    return nt;
}

union mangle_flags {
    u64 raw = 0;
    struct {
        u64 len : 8;
        u64 typ : 4;
        u64 _const : 1;
        u64 _volat : 1;
    };
};

union compound_flags {
    u8 raw = 0;
    struct {
        u8 compiletime : 1;
        u8 reference : 1;
    };
};

union param_flags {
    u8 raw = 0;
    struct {
        u8 compiletime : 1;
        u8 reference : 1;
        u8 spread : 1;
        u8 generic : 1;
        u8 binding : 1;
        u8 thisarg : 1;
    };
};

union ret_flags {
    u8 raw = 0;
    struct {
        u8 compiletime : 1;
        u8 reference : 1;
    };
};

std::string type_table::mangle(type* t) {
    ASSERT(sizeof(mangle_flags) == 8, "mangle_flags type is larger than 8 bytes");
    
    constexpr std::array<u8, 65> required{{
        8, 8, 8, 8, 8, 8, 8, 8,
        8, 8, 8, 8, 8, 8, 8, 8,
        8, 8, 8, 8, 8, 8, 8, 8,
        8, 8, 8, 8, 8, 8, 8, 8,
        4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 4,
        2, 2, 2, 2, 2, 2, 2, 2,
        1, 1, 1, 1, 1, 1, 1, 1, 1
    }};
    
    mangle_flags flags;
    flags.typ = (u64) t->tt;
    flags._const = t->const_flag;
    flags._volat = t->volat_flag;
    
    std::string mangled;
    
    auto add1 = [&mangled](u8 len, u64 data){
        ASSERT(len <= mangled.length() - 8, "Not enough space");
        switch (len) {
            case 1: {
                u8* mdata = reinterpret_cast<u8*>(mangled.data() + 8);
                mdata[0] = data;
                break;
            }
            case 2: {
                u16* mdata = reinterpret_cast<u16*>(mangled.data() + 8);
                mdata[0] = data;
                break;
            }
            case 4: {
                u32* mdata = reinterpret_cast<u32*>(mangled.data() + 8);
                mdata[0] = data;
                break;
            }
            case 8: {
                std::memcpy(mangled.data() + 8, &data, sizeof data);
                break;
            }
        }
    };
    
    auto add = [&mangled](u8 len, const std::vector<u64>& data){
        switch (len) {
            case 1: {
                ASSERT(data.size() <= mangled.length() - 8, "Not enough space");
                u8* mdata = reinterpret_cast<u8*>(mangled.data() + 8);
                for (u64 i = 0; i < data.size(); ++i) {
                    mdata[i] = data[i];
                }
                break;
            }
            case 2: {
                ASSERT(data.size() <= (mangled.length() - 8) / 2, "Not enough space");
                u16* mdata = reinterpret_cast<u16*>(mangled.data() + 8);
                for (u64 i = 0; i < data.size(); ++i) {
                    mdata[i] = data[i];
                }
                break;
            }
            case 4: {
                ASSERT(data.size() <= (mangled.length() - 8) / 4, "Not enough space");
                u32* mdata = reinterpret_cast<u32*>(mangled.data() + 8);
                for (u64 i = 0; i < data.size(); ++i) {
                    mdata[i] = data[i];
                }
                break;
            }
            case 8: {
                ASSERT(data.size() <= (mangled.length() - 8) / 8, "Not enough space");
                std::memcpy(mangled.data() + 8, data.data(), data.size());
                break;
            }
        }
    };
    
    switch (t->tt) {
        case type_type::PRIMITIVE: {
            flags.len = 1;
            mangled.resize(8 + 4);
            
            u16* as_u16 = reinterpret_cast<u16*>(mangled.data() + 8);
            as_u16[0] = (u16) t->primitive.tt;
            as_u16[1] = t->primitive.bits;
            break;
        }
        case type_type::POINTER: {
            flags.len = required[__builtin_clz(t->pointer.at->id)];
            mangled.resize(9 + flags.len);
            add1(flags.len, t->pointer.at->id);
            mangled[8 + flags.len] = (char) t->pointer.tt;
            break;
        }
        case type_type::ARRAY: {
            flags.len = required[std::max(__builtin_clz(t->array.size), __builtin_clz(t->array.at->id))];
            mangled.resize(9 + flags.len * 2);
            add(flags.len, {t->array.sized, t->array.at->id});
            mangled[8 + flags.len * 2] = (char) t->array.sized;
            break;
        }
        case type_type::COMPOUND: {
            std::vector<u64> ids;
            std::vector<compound_flags> cflags;
            u8 max_len = 0;
            u64 members = t->compound.members.size();
            ids.reserve(members + 1);
            cflags.reserve(((members * 2) + 1) / 8);
            ids.push_back(members);
            for (auto& member : t->compound.members) {
                ids.push_back(member.t->id);
                max_len = max_len > required[__builtin_clz(member.t->id)] ? max_len : required[__builtin_clz(member.t->id)]; 
            
                compound_flags& cf = cflags.emplace_back();
                cf.compiletime = member.compiletime;
                cf.reference = member.reference;
            }
            flags.len = max_len;
            mangled.resize(8 + (1 + ids.size()) * flags.len + cflags.size());
            add(max_len, ids);
            
            u64 byte = 8 + (1 + members) * flags.len;
            u8 bit = 0;
            
            std::memset(mangled.data() + byte, 0, mangled.length() - byte);
            
            for (auto& flags : cflags) {
                mangled[byte] |= (flags.raw & 0x3) << bit;
                bit += 2;
                if (bit == 8) {
                    bit = 0;
                    ++byte;
                }
            }
            
            break;
        }
        case type_type::STRUCT: [[fallthrough]];
        case type_type::UNION: [[fallthrough]];
        case type_type::ENUM: [[fallthrough]];
        case type_type::TUPLE: {
            flags.len = required[__builtin_clz(t->scompound.comp->id)];
            mangled.resize(8 + flags.len);
            add1(flags.len, t->scompound.comp->id);
            break;
        }        
        case type_type::FUNCTION: {
            auto& f = t->function;
            u64 paramsize = f.params.size();
            u64 retsize = f.rets.size();
            u64 reservesize = 2 + paramsize + retsize;
            std::vector<u64> ids;
            std::vector<param_flags> pflags;
            std::vector<ret_flags> rflags;
            
            u8 max_len = 0;
            ids.reserve(reservesize);
            ids.push_back(paramsize);
            ids.push_back(retsize);
            pflags.reserve(paramsize);
            rflags.reserve(retsize);
            
            for (auto& tt : f.params) {
                ids.push_back(tt.t->id);
                max_len = max_len > required[__builtin_clz(tt.t->id)] ? max_len : required[__builtin_clz(tt.t->id)];
                param_flags p{0};
                p.compiletime = tt.compiletime;
                p.reference = tt.generic;
                p.spread = tt.spread;
                p.generic = tt.generic;
                p.binding = tt.binding;
                p.thisarg = tt.thisarg;
                pflags.push_back(p);
            }
            for (auto& tt : f.rets) {
                ids.push_back(tt.t->id);
                max_len = max_len > required[__builtin_clz(tt.t->id)] ? max_len : required[__builtin_clz(tt.t->id)];
                ret_flags r{0};
                r.compiletime = tt.compiletime;
                r.reference = tt.reference;
                rflags.push_back(r);
            }
            flags.len = max_len;
            mangled.resize(8 + (flags.len * reservesize) + pflags.size() + ((rflags.size() + 3) / 4));
            add(max_len, ids);
            
            u64 byte = 8 + (flags.len * reservesize);
            u8 bit = 0;
            
            std::memset(mangled.data() + byte, 0, mangled.length() - byte);
            
            for (auto& pf : pflags) {
                mangled[byte] = pf.raw & 0x1F;
                ++byte;
            }
            
            for (auto& rf : rflags) {
                mangled[byte] |= (rf.raw & 0x3) << bit;
                bit += 2;
                if (bit == 8) {
                    bit = 0;
                    ++byte;
                }
            }
            
            break;
        }
        case type_type::SUPERFUNCTION: {
            flags.len = required[__builtin_clz(t->sfunction.function->id)];
            mangled.resize(8 + flags.len);
            add1(flags.len, t->sfunction.function->id);
            break;
        }
        case type_type::SPECIAL: {
            flags.len = 1;
            mangled.resize(8 + 1);
            mangled[8] = (char) t->special.tt;
            break;
        }
    }
    
    std::memcpy(mangled.data(), &flags, sizeof flags);
    
    return mangled;
}

template <typename T>
void unmangle_internal(type_table& tt, const char* d, type_type t, type* dest) {
    const T* as_t = reinterpret_cast<const T*>(d);
    
    dest->tt = t;
    
    switch (t) {
        case type_type::PRIMITIVE: {
            const u16* as_u16 = reinterpret_cast<const u16*>(d);
            dest->primitive = type_primitive{(primitive_type) as_u16[0], as_u16[1]};
            break;
        }
        case type_type::POINTER: {
            ASSERT(tt[as_t[0]], "Type id pointed nowhere");
            dest->pointer = type_pointer{(pointer_type) d[sizeof(T)], tt[as_t[0]]};
            break;
        }
        case type_type::ARRAY: {
            ASSERT(tt[as_t[0]], "Type id pointed nowhere");
            ASSERT(tt[as_t[1]], "Type id pointed nowhere");
            dest->array = type_array{tt[as_t[1]], (bool) tt[as_t[0]], d[sizeof(T) * 2] > 0};
            break;
        }
        case type_type::COMPOUND: {
            u64 size = as_t[0];
            std::vector<member> ts;
            ts.reserve(size);
            
            u64 byte = (size + 1) * sizeof(T);
            u8 bit = 0;
            
            for (u64 i = 0; i < size; ++i) {
                ASSERT(tt[as_t[1 + i]], "Type id pointed nowhere");
                compound_flags flgs{(u8) ((d[byte] >> bit) & 0x3)};
                ts.push_back({tt[as_t[1 + i]], flgs.compiletime, flgs.reference});
                bit += 2;
                if (bit == 8) {
                    bit = 0;
                    ++byte;
                }
            }
            dest->compound = type_compound{ts};
            break;
        }
        case type_type::STRUCT: [[fallthrough]];
        case type_type::UNION: [[fallthrough]];
        case type_type::ENUM: [[fallthrough]];
        case type_type::TUPLE: {
            ASSERT(tt[as_t[0]], "Type id pointed nowhere");
            dest->scompound = type_supercompound{tt[as_t[0]], false, false}; // TODO Generic/Generated
            break;
        }        
        case type_type::FUNCTION: {
            u64 psize = as_t[0];
            u64 rsize = as_t[1];
            std::vector<param> params{};
            params.reserve(psize);
            std::vector<ret> rets{};
            rets.reserve(rsize);
            
            u64 byte = (2 + psize + rsize) * sizeof(T);
            u8 bit = 0;
            
            for (u64 i = 0; i < psize; ++i) {
                ASSERT(tt[as_t[2 + i]], "Type id pointed nowhere");
                param_flags flgs{(u8) (d[byte] & 0x1F)};
                params.push_back(
                    {tt[as_t[2 + i]], flgs.compiletime, flgs.reference, flgs.spread, flgs.generic, flgs.binding, flgs.thisarg}
                );
                ++byte;
            }
            
            for (u64 i = 0; i < rsize; ++i) {
                ASSERT(tt[as_t[2 + psize + i]], "Type id pointed nowhere");
                ret_flags retflg{(u8) ((d[byte] >> bit) & 0x3)};
                rets.push_back({tt[as_t[2 + psize + i]], retflg.compiletime, retflg.reference});
                bit += 2;
                if (bit == 8) {
                    bit = 0;
                    ++byte;
                }
            }
            
            break;
        }
        case type_type::SUPERFUNCTION: {
            ASSERT(tt[as_t[0]], "Type id pointed nowhere");
            dest->sfunction = type_superfunction{tt[as_t[0]], {}, {}};
            break;
        }
        case type_type::SPECIAL: {
            dest->special = type_special{(special_type) d[0]};
            break;
        }
    }
}

type type_table::unmangle(const std::string& s) {
    ASSERT(s.length() > 8, "Invalid mangled string");
    mangle_flags flgs{};
    const char* data = s.data();
    std::memcpy(&flgs, data, sizeof flgs);
    type t{*this, 0xFFFFFFFF, type_special{special_type::NONE}, flgs._const > 0, flgs._volat > 0};
    
    switch (flgs.len) {
        case 1:
            unmangle_internal<u8>(*this, data + 8, (type_type) flgs.typ, &t);
            break;
        case 2:
            unmangle_internal<u16>(*this, data + 8, (type_type) flgs.typ, &t);
            break;
        case 4:
            unmangle_internal<u32>(*this, data + 8, (type_type) flgs.typ, &t);
            break;
        case 8:
            unmangle_internal<u64>(*this, data + 8, (type_type) flgs.typ, &t);
            break;
    }
    return t;
}

type* type_table::get(type* t) {
    return mangle_table[mangle(t)];
}

type* type_table::get_or_add(type* t) {
    type* tt = get(t);
    if (!tt) {
        return add(t);
    } else {
        return tt;
    }
}
