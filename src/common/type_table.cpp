#include "common/type_table.h"

#include <cstring>
#include <iomanip>
#include <string>
#include <sstream>
#include "common/ast.h"

type_table::type_table() {
    add_type(t_void);
    add_type(t_byte);
    add_type(t_short);
    add_type(t_int);
    add_type(t_long);
    add_type(t_sig);
    add_type(t_float);
    add_type(t_double);
    add_type(t_bool);
    add_type(t_char);
    add_type(t_string);
    add_type(t_fun);
    add_type(t_let);
    add_type(t_null);
    add_type(t_nothing);
}

type_table::~type_table() {
    // Do not delete the basic types, they go when the program goes
    for (u64 i = etype_ids::LAST + 1; i < types.size(); ++i) {
        if (types[i]) {
            delete types[i];
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
            }
            add_bytes(ss, p.t->id);
            ss << (char) t->flags;
            return ss.str();
        }
        case ettype::COMBINATION: {
            auto& c = t->as_combination();
            ss << mangle_bytes::combination_start;
            add_bytes(ss, (u64) c.types.size());
            for (auto& typ : c.types) {
                add_bytes(ss, typ->id);
                ss << (char) typ->flags;
            }
            return ss.str();
        }
        case ettype::PFUNCTION: {
            auto& pf = t->as_pfunction();
            ss << mangle_bytes::function_start << (char) t->flags;
            add_bytes(ss, pf.rets->id);
            for (auto& p : pf.params) {
                ss << mangle_bytes::function_param;
                add_bytes(ss, p.t->id);
                ss << (char) p.t->flags << (char) p.flags;
            }
            return ss.str();
        }
        case ettype::PSTRUCT: {
            auto& ps = t->as_pstruct();
            ss << mangle_bytes::struct_start << (char) t->flags;
            for (auto& f : ps.fields) {
                ss << mangle_bytes::struct_separator;
                add_bytes(ss, f.t->id);
                ss << (char) f.t->flags << (char) f.bits;
            }
            return ss.str();
        }
        default:
            ss << mangle_bytes::normal_start;
            add_bytes(ss, t->id);
            ss << (char) t->flags;
            return ss.str();
    }
}

std::string type_table::mangle_pure(type* t) {
    using namespace std::string_literals;
    
    std::stringstream ss{};
    switch (t->tt) {
        case ettype::FUNCTION: {
            auto& f = t->as_function();
            ss << mangle_bytes::function_start << (char) t->flags;
            add_bytes(ss, f.rets->id);
            for (auto& p : f.params) {
                ss << mangle_bytes::function_param;
                add_bytes(ss, p.t->id);
                ss << (char) p.t->flags << (char) p.flags;
            }
            return ss.str();
        }
        case ettype::STRUCT: {
            auto& s = t->as_struct();
            ss << mangle_bytes::struct_start << (char) t->flags;
            for (auto& f : s.fields) {
                ss << mangle_bytes::struct_separator;
                add_bytes(ss, f.t->id);
                ss << (char) f.t->flags << (char) f.bits;
            }
            return ss.str();
        }
        case ettype::UNION: {
            auto& s = t->as_union();
            ss << mangle_bytes::union_start << (char) t->flags;
            for (auto& f : s.fields) {
                ss << mangle_bytes::struct_separator;
                add_bytes(ss, f.t->id);
                ss << (char) f.t->flags << (char) 0;
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
            std::memcpy(&cti.str[0], mangled.data() + 1, sizeof(type_id));
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
            type_flags flags{0};
            if (mangled[0] == mangle_bytes::array_ptr_start) {
                ptr_type = eptr_type::ARRAY;
                std::memcpy(&cti.str[0], mangled.data() + 2, sizeof(type_id));
                size = cti.id;
                std::memcpy(&cti.str[0], mangled.data() + (2 + sizeof(type_id)), sizeof(type_id));
                pointed = cti.id;
                flags = (type_flags) mangled.data()[2 + sizeof(type_id) * 2];
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
                std::memcpy(&cti.str[0], mangled.data() + 7, sizeof(type_id));
                pointed = cti.id;
                flags = (type_flags) mangled.data()[7 + sizeof(type_id)];
            }
            type* tpointed = get(pointed, flags);
            type t{ettype::POINTER, etype_ids::LET, ptr_flags, type_pointer{ptr_type, size, tpointed}};
            return t;
        }
        case mangle_bytes::struct_start: {
            type_flags flags = (u8) mangled[1];
            type t{ettype::PSTRUCT, etype_ids::LET, flags, type_pstruct{{}}};
            auto& fields = t.as_pstruct().fields;
            u64 pt{2};
            while (pt < mangled.length()) {
                if (mangled[pt] != mangle_bytes::struct_separator) {
                    break;
                }
                std::memcpy(&cti.str[0], mangled.data() + (pt + 1), sizeof(type_id));
                type* field = get(cti.id, mangled.data()[pt + 1 + sizeof(type_id)]);
                fields.push_back({field, (type_flags) mangled[pt + 2 + sizeof(type_id)]});
                pt += 11;
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
                std::memcpy(&cti.str[0], mangled.data() + (pt + 1), sizeof(type_id));
                type* field = get(cti.id, mangled.data()[pt + 1 + sizeof(type_id)]);
                fields.push_back({field});
                pt += 11;
            }
            return t;
        }
        case mangle_bytes::combination_start: {
            type t{ettype::COMBINATION, etype_ids::LET, 0, type_combination{}};
            auto& typs = t.as_combination().types;
            std::memcpy(&cti.str[0], mangled.data() + 1, sizeof(u64));
            u64 size = cti.id;
            for (u64 i = 0; i < size; ++i) {
                std::memcpy(&cti.str[0], mangled.data() + (1 + sizeof(type_id) * (i + 1)), sizeof(type_id));
                type* typ = get(cti.id, mangled.data()[1 + sizeof(type_id) * (i + 1) + sizeof(type_id)]);
                typs.push_back(typ);
            }
            return t;
        }
        case mangle_bytes::function_start: {
            type_flags flags = (u8) mangled[1];
            type t{ettype::PFUNCTION, etype_ids::FUN, flags, type_pfunction{}};
            auto& rets = t.as_pfunction().rets;
            auto& params = t.as_pfunction().params;
            std::memcpy(&cti.str[0], mangled.data() + 2, sizeof(type_id));
            rets = types[cti.id];
            u64 pt{10};
            while (pt < mangled.length()) {
                if (mangled[pt] != mangle_bytes::function_param) {
                    break;
                }
                std::memcpy(&cti.str[0], mangled.data() + (pt + 1), sizeof(type_id));
                type* param = get(cti.id, mangled.data()[pt + 1 + sizeof(type_id)]);
                params.push_back({param, (type_flags) mangled[pt + 2 + sizeof(type_id)]});
                pt += 11;
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
    if (t->has_special_flags()) {
        type nt = *t;
        nt.flags = t->get_default_flags();
        type* def = get_or_add(nt);
        t->id = def->id;
    } else {
        t->id = next_id();
    }
    u64 pos = types.size();
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
    mangle_table.insert({mangled, pos});
    return t;
}

type* type_table::add_type(const std::string& mangled) {
    type* t = new type{unmangle(mangled)};
    t->id = next_id();
    types.push_back(t);
    mangle_table.insert({mangled, t->id});
    return t;
}

type* type_table::update_type(type_id id, type& nvalue) {
    type* toup = types[id];
    std::string mangl = mangle(toup);
    mangle_table.erase(mangl);
    toup->tt = nvalue.tt;
    toup->flags = nvalue.flags;
    toup->t = nvalue.t;
    
    if (toup->tt == ettype::FUNCTION || toup->tt == ettype::STRUCT) {
        mangl = mangle(toup);
        std::string manglp = mangle_pure(toup);
        type* ptp = get(manglp);
        if (!ptp) {
            ptp = add_type(manglp);
        }
        if (toup->tt == ettype::STRUCT) {
            toup->as_struct().pure = &ptp->as_pstruct();
        } else {
            toup->as_function().pure = &ptp->as_pfunction();
        }
    } else {
        mangl = mangle(toup);
    }
    
    mangle_table.insert({mangl, toup->id});
    return toup;
}

type* type_table::get(type_id id, type_flags flags) {
    if (id >= types.size()) {
        return nullptr;
    }
    type t = *types[id];
    t.flags = flags;
    return get(t);
}

type* type_table::get(type& t) {
    return get(mangle(&t));
}

type* type_table::get(const std::string& mangled) {
    auto t = mangle_table.find(mangled);
    if (t == mangle_table.end() || t->second == 0) { // Cannot pull VOID from table
        return nullptr;
    } else {
        return types[t->second];
    }
}

type* type_table::get_or_add(type& t) {
    type* ret = get(t);
    if (!ret) {
        ret = add_type(t);
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

std::string type_table::print() {
    std::stringstream ss{};
    ss << "~~ Types ~~\n\n";
    for (u64 i = 0; i < types.size(); ++i) {
        ss << "[" << i << "]:\n" << types[i]->print() << "\n\n";
    }
    
    ss << "~~ Mangles ~~\n\n" << std::setfill('0');
    for (auto& [name, id] : mangle_table) {
        ss <<  std::hex;
        for (char c : name) {
            ss << std::setw(2) << (u16) c;
        }
        ss << std::dec << ": " << id << "\n";
    }
    return ss.str();
}

type* type_table::t_void = new type{ettype::PRIMITIVE, etype_ids::VOID, 0};
type* type_table::t_byte = new type{ettype::PRIMITIVE, etype_ids::BYTE, 0};
type* type_table::t_short = new type{ettype::PRIMITIVE, etype_ids::SHORT, etype_flags::SIGNED};
type* type_table::t_int = new type{ettype::PRIMITIVE, etype_ids::INT, etype_flags::SIGNED};
type* type_table::t_long = new type{ettype::PRIMITIVE, etype_ids::LONG, etype_flags::SIGNED};
type* type_table::t_sig = new type{ettype::PRIMITIVE, etype_ids::SIG, 0};
type* type_table::t_float = new type{ettype::PRIMITIVE, etype_ids::FLOAT, 0};
type* type_table::t_double = new type{ettype::PRIMITIVE, etype_ids::DOUBLE, 0};
type* type_table::t_bool = new type{ettype::PRIMITIVE, etype_ids::BOOL, 0};
type* type_table::t_char = new type{ettype::PRIMITIVE, etype_ids::CHAR, 0};
type* type_table::t_string = new type{ettype::PRIMITIVE, etype_ids::STRING, 0};
type* type_table::t_fun = new type{ettype::PRIMITIVE, etype_ids::FUN, 0};
type* type_table::t_let = new type{ettype::PRIMITIVE, etype_ids::LET, 0};
type* type_table::t_null = new type{ettype::PRIMITIVE, etype_ids::NNULL, 0};
type* type_table::t_nothing = new type{ettype::PRIMITIVE, etype_ids::NOTHING, 0};
