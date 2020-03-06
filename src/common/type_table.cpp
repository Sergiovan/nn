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
    C8 = add_primitive({primitive_type::CHARACTER, 8}, false, false);
    C16 = add_primitive({primitive_type::CHARACTER, 16}, false, false);
    C32 = add_primitive({primitive_type::CHARACTER, 32}, false, false);
    F32 = add_primitive({primitive_type::FLOATING, 32}, false, false);
    F64 = add_primitive({primitive_type::FLOATING, 64}, false, false);
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
}

type* type_table::add_primitive(const type_primitive& p, const bool _const, const bool volat) {
    type t {*this, types.size(), p, _const, volat};
    return get_or_add(&t);
}

type* type_table::add_pointer(const type_pointer& p, const bool _const, const bool volat) {
    type t {*this, types.size(), p, _const, volat};
    return get_or_add(&t);
}

type* type_table::add_array(const type_array& a, const bool _const, const bool volat) {
    type t {*this, types.size(), a, _const, volat};
    return get_or_add(&t);
}

type* type_table::add_compound(const type_compound& c, const bool _const, const bool volat) {
    type t {*this, types.size(), c, _const, volat};
    return get_or_add(&t);
}

type* type_table::add_supercompound(const type_supercompound& sc, type_type sct, const bool _const, const bool volat) {
    type t {*this, types.size(), sct, sc, _const, volat};
    return get_or_add(&t);
}

type* type_table::add_function(const type_function& f, const bool _const, const bool volat) {
    type t {*this, types.size(), f, _const, volat};
    return get_or_add(&t);
}

type* type_table::add_superfunction(const type_superfunction& sf, const bool _const, const bool volat) {
    type t {*this, types.size(), sf, _const, volat};
    return get_or_add(&t);
}

type* type_table::add_special(const type_special& s, const bool _const, const bool volat) {
    type t {*this, types.size(), s, _const, volat};
    return get_or_add(&t);
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

type* type_table::array_of(type* t, bool _const, bool volat) {
    return add_array({t, false, 0}, _const, volat);
}

type* type_table::sized_array_of(type* t, u64 size, bool _const, bool volat) {
    return add_array({t, true, size}, _const, volat);
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

union param_flags {
    u8 raw = 0;
    struct {
        u8 compiletime : 1;
        u8 spread : 1;
        u8 generic : 1;
        u8 thisarg : 1;
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
        ASSERT(len < mangled.length() - 8, "Not enough space");
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
                ASSERT(data.size() < mangled.length() - 8, "Not enough space");
                u8* mdata = reinterpret_cast<u8*>(mangled.data() + 8);
                for (u64 i = 0; i < data.size(); ++i) {
                    mdata[i] = data[i];
                }
                break;
            }
            case 2: {
                ASSERT(data.size() < (mangled.length() - 8) / 2, "Not enough space");
                u16* mdata = reinterpret_cast<u16*>(mangled.data() + 8);
                for (u64 i = 0; i < data.size(); ++i) {
                    mdata[i] = data[i];
                }
                break;
            }
            case 4: {
                ASSERT(data.size() < (mangled.length() - 8) / 4, "Not enough space");
                u32* mdata = reinterpret_cast<u32*>(mangled.data() + 8);
                for (u64 i = 0; i < data.size(); ++i) {
                    mdata[i] = data[i];
                }
                break;
            }
            case 8: {
                ASSERT(data.size() < (mangled.length() - 8) / 8, "Not enough space");
                std::memcpy(mangled.data() + 8, data.data(), data.size());
                break;
            }
        }
    };
    
    switch (t->tt) {
        case type_type::PRIMITIVE: {
            flags.len = 1;
            mangled.reserve(8 + 4);
            
            u16* as_u16 = reinterpret_cast<u16*>(mangled.data() + 8);
            as_u16[0] = (u16) t->primitive.tt;
            as_u16[1] = t->primitive.bits;
            break;
        }
        case type_type::POINTER: {
            flags.len = required[__builtin_clz(t->pointer.at->id)];
            mangled.reserve(9 + flags.len);
            add1(flags.len, t->pointer.at->id);
            mangled[8 + flags.len] = (char) t->pointer.tt;
            break;
        }
        case type_type::ARRAY: {
            flags.len = required[std::max(__builtin_clz(t->array.sized), __builtin_clz(t->array.at->id))];
            mangled.reserve(9 + flags.len * 2);
            add(flags.len, {t->array.sized, t->array.at->id});
            mangled[8 + flags.len * 2] = (char) t->array.sized;
            break;
        }
        case type_type::COMPOUND: {
            std::vector<u64> ids;
            u8 max_len = 0;
            ids.reserve(t->compound.elems.size() + 1);
            ids.push_back(t->compound.elems.size());
            for (type* tt : t->compound.elems) {
                ids.push_back(tt->id);
                max_len = max_len > required[__builtin_clz(tt->id)] ? max_len : required[__builtin_clz(tt->id)];
            }
            add(max_len, ids);
            break;
        }
        case type_type::STRUCT: [[fallthrough]];
        case type_type::UNION: [[fallthrough]];
        case type_type::ENUM: [[fallthrough]];
        case type_type::TUPLE: {
            flags.len = required[__builtin_clz(t->scompound.comp->id)];
            mangled.reserve(8 + flags.len);
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
            std::vector<u8> rflags; // Really a vector<bool> 
            
            u8 max_len = 0;
            ids.reserve(reservesize);
            ids.push_back(paramsize);
            ids.push_back(retsize);
            pflags.reserve(paramsize);
            rflags.reserve(retsize);
            
            for (auto& tt : f.params) {
                ids.push_back(tt.t->id);
                max_len = max_len > required[__builtin_clz(tt.t->id)] ? max_len : required[__builtin_clz(tt.t->id)];
                param_flags p;
                p.compiletime = tt.compiletime;
                p.spread = tt.spread;
                p.generic = tt.generic;
                p.thisarg = tt.thisarg;
                pflags.push_back(p);
            }
            for (auto& tt : f.rets) {
                ids.push_back(tt.t->id);
                max_len = max_len > required[__builtin_clz(tt.t->id)] ? max_len : required[__builtin_clz(tt.t->id)];
                rflags.push_back(tt.compiletime);
            }
            add(max_len, ids);
            
            u64 byte = 8 + reservesize;
            u8 bit = 0;
            
            std::memset(mangled.data() + byte, 0, mangled.length() - byte);
            
            for (auto& pf : pflags) {
                mangled[byte] |= (pf.raw & 0xF) << bit;
                bit += 4;
                if (bit == 8) {
                    bit = 0;
                    ++byte;
                }
            }
            
            for (auto& rf : rflags) {
                mangled[byte] |= (rf & 1) << bit;
                ++bit;
                if (bit == 8) {
                    bit = 0;
                    ++byte;
                }
            }
            
            break;
        }
        case type_type::SUPERFUNCTION: {
            flags.len = required[__builtin_clz(t->sfunction.function->id)];
            mangled.reserve(8 + flags.len);
            add1(flags.len, t->sfunction.function->id);
            break;
        }
        case type_type::SPECIAL: {
            flags.len = 1;
            mangled.reserve(8 + 1);
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
            dest->array = type_array{tt[as_t[1]], tt[as_t[0]], d[sizeof(T) * 2] > 0};
            break;
        }
        case type_type::COMPOUND: {
            u64 size = as_t[0];
            std::vector<type*> ts;
            ts.reserve(size);
            for (u64 i = 0; i < size; ++i) {
                ASSERT(tt[as_t[1 + i]], "Type id pointed nowhere");
                ts.push_back(tt[as_t[1 + i]]);
            }
            dest->compound = type_compound{ts};
            break;
        }
        case type_type::STRUCT: [[fallthrough]];
        case type_type::UNION: [[fallthrough]];
        case type_type::ENUM: [[fallthrough]];
        case type_type::TUPLE: {
            ASSERT(tt[as_t[0]], "Type id pointed nowhere");
            dest->scompound = type_supercompound{tt[as_t[0]]};
            break;
        }        
        case type_type::FUNCTION: {
            u64 psize = as_t[0];
            u64 rsize = as_t[1];
            std::vector<param> params{};
            params.reserve(psize);
            std::vector<ret> rets{};
            rets.reserve(rsize);
            
            u64 byte = psize + rsize;
            u8 bit = 0;
            
            for (u64 i = 0; i < psize; ++i) {
                ASSERT(tt[as_t[2 + i]], "Type id pointed nowhere");
                param_flags flgs{(u8) ((d[byte] >> bit) & 0xF)};
                params.push_back(
                    {tt[as_t[2 + i]], flgs.compiletime, flgs.spread, flgs.generic, flgs.thisarg}
                );
                bit += 4;
                if (bit == 8) {
                    bit = 0;
                    ++byte;
                }
            }
            
            for (u64 i = 0; i < rsize; ++i) {
                ASSERT(tt[as_t[2 + psize + i]], "Type id pointed nowhere");
                u8 retflg = (d[byte] >> bit) & 1;
                rets.push_back({tt[as_t[2 + psize + i]], retflg});
                ++bit;
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
