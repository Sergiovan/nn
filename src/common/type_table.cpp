#include "common/type_table.h"

#include <cstring>
#include <string>
#include <sstream>
#include "common/ast.h"

type_table::type_table() {
    for (type_id i = etype_ids::VOID; i <= etype_ids::NOTHING; ++i) {
        type* t = type::primitive();
        t->as_primitive().t = i;
        if (i == etype_ids::SHORT || i == etype_ids::INT || i == etype_ids::LONG) {
            t->flags |= etype_flags::SIGNED;
        }
        add_type(t);
    } // Add all primitives
    
    t_void = types[etype_ids::VOID];
    t_byte = types[etype_ids::BYTE];
    t_short = types[etype_ids::SHORT];
    t_int = types[etype_ids::INT];
    t_long = types[etype_ids::LONG];
    t_sig = types[etype_ids::SIG];
    t_float = types[etype_ids::FLOAT];
    t_double = types[etype_ids::DOUBLE];
    t_bool = types[etype_ids::BOOL];
    t_char = types[etype_ids::CHAR];
    t_string = types[etype_ids::STRING];
    t_fun = types[etype_ids::FUN];
    t_let = types[etype_ids::LET];
    t_null = types[etype_ids::NNULL];
    t_nothing = types[etype_ids::NOTHING];
}

type_table::~type_table() {
    for (auto& t : types) {
        if (t) {
            delete t;
        }
    }
}

template<typename T>
void add_bytes(std::stringstream& ss, T t) {
    char* bytes = reinterpret_cast<char*>(&t);
    for (u64 i = 0; i < sizeof(T); ++i) {
        ss << (char) bytes[i];
    }
}

std::string type_table::mangle(type* t) {
    using namespace std::string_literals;
    
    std::stringstream ss{};
    switch (t->tt) {
        case ettype::POINTER: {
            auto& p = t->as_pointer();
            switch (p.ptr_t) {
                case eptr_type::NAKED:
                    ss << mangle_bytes::naked_ptr_start;
                    break;
                case eptr_type::UNIQUE:
                    ss << mangle_bytes::unique_ptr_start;
                    break;
                case eptr_type::SHARED:
                    ss << mangle_bytes::shared_ptr_start;
                    break;
                case eptr_type::WEAK:
                    ss << mangle_bytes::weak_ptr_start;
                    break;
                case eptr_type::ARRAY:
                    ss << mangle_bytes::array_ptr_start;
                    break;
            }
            
            ss << (char) t->flags;
            
            if (p.ptr_t == eptr_type::ARRAY) {
                add_bytes(ss, p.size);
                ss << "\0\0\0\0\0\0"s;
            } else {
                ss << "\0\0\0\0\0"s;
            }
            add_bytes(ss, p.t->id);
            return ss.str();
        }
        case ettype::COMBINATION: {
            auto& c = t->as_combination();
            ss << mangle_bytes::combination_start;
            add_bytes(ss, (u64) c.types.size());
            for (auto& typ : c.types) {
                add_bytes(ss, typ->id);
            }
        }
        case ettype::PFUNCTION: {
            auto& pf = t->as_pfunction();
            ss << mangle_bytes::function_start << (char) t->flags;
            add_bytes(ss, pf.rets->id);
            for (auto& p : pf.params) {
                ss << mangle_bytes::function_param;
                add_bytes(ss, p.t->id);
                ss << (char) p.flags;
            }
            return ss.str();
        }
        case ettype::PSTRUCT: {
            auto& ps = t->as_pstruct();
            ss << mangle_bytes::struct_start << (char) t->flags;
            for (auto& f : ps.fields) {
                ss << mangle_bytes::struct_separator;
                add_bytes(ss, f.t->id);
                ss << (char) f.bits;
            }
            return ss.str();
        }
        default:
            ss << mangle_bytes::normal_start;
            add_bytes(ss, t->id);
            ss << (char) t->flags << "\0\0\0\0\0\0"s;
            return ss.str();
    }
}

std::string type_table::mangle_pure(type* t) {
    using namespace std::string_literals;
    
    std::stringstream ss{};
    switch (t->tt) {
        case ettype::FUNCTION: {
            auto& f = t->as_function();
            auto pf = f.pure;
            ss << mangle_bytes::function_start << (char) t->flags;
            add_bytes(ss, pf->rets->id);
            for (auto& p : pf->params) {
                ss << mangle_bytes::function_param;
                add_bytes(ss, p.t->id);
                ss << (char) p.flags;
            }
            return ss.str();
        }
        case ettype::STRUCT: {
            auto& s = t->as_struct();
            ss << mangle_bytes::struct_start << (char) t->flags;
            for (auto& f : s.pure->fields) {
                ss << mangle_bytes::struct_separator;
                add_bytes(ss, f.t->id);
                ss << (char) f.bits;
            }
            return ss.str();
        }
        case ettype::UNION: {
            auto& s = t->as_union();
            ss << mangle_bytes::union_start << (char) t->flags;
            for (auto& f : s.fields) {
                ss << mangle_bytes::struct_separator;
                add_bytes(ss, f.t->id);
                ss << (char) 0;
            }
            return ss.str();
        }
        default:
            return {};
    }
}

type type_table::unmangle(const std::string& mangled) {
    union {
        char str[sizeof(type_id)];
        type_id id;
    } cti;
    
    switch (mangled[0]) {
        case mangle_bytes::normal_start: {
            std::memcpy(&cti.str, &mangled[1], sizeof(type_id));
            type_id id = cti.id;
            type t = *types[id];
            t.flags = (u8) mangled[1 + sizeof(type_id)];
            return t;
        }
        case mangle_bytes::naked_ptr_start: [[fallthrough]];
        case mangle_bytes::unique_ptr_start: [[fallthrough]];
        case mangle_bytes::shared_ptr_start: [[fallthrough]];
        case mangle_bytes::weak_ptr_start: [[fallthrough]];
        case mangle_bytes::array_ptr_start: {
            type_flags ptr_flags = (u8) mangled[1];
            u64 size{0};
            eptr_type ptr_type = eptr_type::NAKED;
            type_id pointed{0};
            if (mangled[0] == mangle_bytes::array_ptr_start) {
                ptr_type = eptr_type::ARRAY;
                std::memcpy(&cti.str, &mangled[2], sizeof(type_id));
                size = cti.id;
                std::memcpy(&cti.str, &mangled[2 + sizeof(type_id) + 6], sizeof(type_id));
                pointed = cti.id;
            } else {
                if (mangle_bytes::naked_ptr_start == mangled[0]) {
                    ptr_type = eptr_type::NAKED;
                } else if (mangle_bytes::unique_ptr_start == mangled[0]) {
                    ptr_type = eptr_type::UNIQUE;
                } else if (mangle_bytes::shared_ptr_start == mangled[0]) {
                    ptr_type = eptr_type::SHARED;
                } else if (mangle_bytes::weak_ptr_start == mangled[0]) {
                    ptr_type = eptr_type::WEAK;
                }
                std::memcpy(&cti.str, &mangled[7], sizeof(type_id));
                pointed = cti.id;
            }
            type* tpointed = types[pointed];
            type t{ettype::POINTER, etype_ids::LET, ptr_flags, type_pointer{ptr_type, size, tpointed}};
            return t;
        }
        case mangle_bytes::struct_start: {
            type_flags flags = (u8) mangled[1];
            type t{ettype::PSTRUCT, etype_ids::LET, flags, type_pstruct{{}}};
            auto& fields = t.as_pstruct().fields;
            u64 pt{0};
            while (pt < mangled.length()) {
                if (mangled[pt] != mangle_bytes::struct_separator) {
                    break;
                }
                std::memcpy(&cti.str, &mangled[pt + 1], sizeof(type_id));
                fields.push_back({types[cti.id], (type_flags) mangled[pt + 1 + sizeof(type_id)]});
                pt += 10;
            }
            return t;
        }
        case mangle_bytes::union_start: {
            type_flags flags = (u8) mangled[1];
            type t{ettype::UNION, etype_ids::LET, flags, type_union{{}, nullptr}};
            auto& fields = t.as_union().fields;
            u64 pt{2};
            while (pt < mangled.length()) {
                if (mangled[pt] != mangle_bytes::struct_separator) {
                    break;
                }
                std::memcpy(&cti.str, &mangled[pt + 1], sizeof(type_id));
                fields.push_back({types[cti.id]});
                pt += 10;
            }
            return t;
        }
        case mangle_bytes::combination_start: {
            type t{ettype::COMBINATION, etype_ids::LET, 0, type_combination{}};
            auto& typs = t.as_combination().types;
            std::memcpy(&cti.str, &mangled[1], sizeof(u64));
            u64 size = cti.id;
            for (u64 i = 0; i < size; ++i) {
                std::memcpy(&cti.str, &mangled[1 + sizeof(u64) * (i + 1)], sizeof(u64));
                typs.push_back(types[cti.id]);
            }
            return t;
        }
        case mangle_bytes::function_start: {
            type_flags flags = (u8) mangled[1];
            type t{ettype::PFUNCTION, etype_ids::FUN, flags, type_pfunction{}};
            auto& rets = t.as_pfunction().rets;
            auto& params = t.as_pfunction().params;
            std::memcpy(&cti.str, &mangled[2], sizeof(type_id));
            rets = types[cti.id];
            u64 pt{9};
            while (pt < mangled.length()) {
                if (mangled[pt] != mangle_bytes::function_param) {
                    break;
                }
                std::memcpy(&cti.str, &mangled[pt + 1], sizeof(type_id));
                params.push_back({types[cti.id], (type_flags) mangled[pt + 1 + sizeof(type_id)]});
                pt += 10;
            }
            return t;
        }
        default:
            return {};
    }
}

type* type_table::add_type(type& t) {
    return add_type(new type{t});
}

type* type_table::add_type(type* t) {
    t->id = next_id();
    types.push_back(t);
    std::string mangled{};
    if (t->tt == ettype::PFUNCTION || t->tt == ettype::PSTRUCT) {
        mangled = mangle(t);
        if (mangle_table.find(mangled) != mangle_table.end()) {
            return nullptr; // Already in
        }
    } else if (t->tt == ettype::FUNCTION || t->tt == ettype::STRUCT) {
        mangled = mangle(t);
        std::string pure_mangled = mangle_pure(t);
        type* ptp = get(pure_mangled);
        if (!ptp) {
            ptp = add_type(pure_mangled);
        }
        if (t->tt == ettype::STRUCT) {
            t->as_struct().pure = &ptp->as_pstruct();
        } else {
            t->as_function().pure = &ptp->as_pfunction();
        }
    } else {
        mangled = mangle(t);
    }
    mangle_table.insert({mangled, t->id});
    return t;
}

type* type_table::add_type(const std::string& mangled) {
    type* t = new type{unmangle(mangled)};
    t->id = next_id();
    types.push_back(t);
    mangle_table.insert({mangled, t->id});
    return t;
}

type* type_table::get(type_id id) {
    return types[id];
}

type* type_table::get(type& t) {
    return get(mangle(&t));
}

type* type_table::get(const std::string& mangled) {
    auto t = mangle_table.find(mangled);
    if (t == mangle_table.end()) {
        return nullptr;
    } else {
        return types[t->second];
    }
}

type* type_table::get_or_add(type& t) {
    type* ret = get(t);
    if (!ret) {
        ret = add_type(&t);
    }
    return ret;
}

type_id type_table::next_id() {
    return types.size();
}

void type_table::merge(type_table&& o) {
    for (auto& t : o.types) {
        if (add_type(t)) {
            t = nullptr;
        }
    }
    o.types.clear();
    o.mangle_table.clear();
}
