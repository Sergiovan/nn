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
        case ettype::PFUNCTION: {
            auto& pf = t->as_pfunction();
            ss << mangle_bytes::function_start << (char) t-> flags;
            for (auto t : pf.returns) {
                ss << mangle_bytes::function_return;
                add_bytes(ss, t->id);
            }
            for (auto& p : pf.params) {
                ss << mangle_bytes::function_param;
                add_bytes(ss, p.t->id);
                ss << (char) p.flags;
            }
            return ss.str();
        }
        case ettype::PSTRUCT: {
            return {};
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
            for (auto t : pf->returns) {
                ss << mangle_bytes::function_return;
                add_bytes(ss, t->id);
            }
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
            for (auto& f : s.fields) {
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
            type t{ettype::STRUCT, etype_ids::LET, flags, type_struct{{}, nullptr}};
            auto& fields = t.as_struct().fields;
            u64 pt{0};
            while (pt < mangled.length()) {
                if (mangled[pt] != mangle_bytes::struct_separator) {
                    break;
                }
                std::memcpy(&cti.str, &mangled[pt + 1], sizeof(type_id));
                fields.push_back({types[cti.id], nullptr, (type_flags) mangled[pt + 1 + sizeof(type_id)]});
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
        case mangle_bytes::function_start: {
            type_flags flags = (u8) mangled[1];
            type t{ettype::PFUNCTION, etype_ids::FUN, flags, type_pfunction{}};
            auto& rets = t.as_pfunction().returns;
            auto& params = t.as_pfunction().params;
            u64 pt{2};
            while (pt < mangled.length()) {
                if (mangled[pt] != mangle_bytes::function_return) {
                    break;
                }
                std::memcpy(&cti.str, &mangled[pt + 1], sizeof(type_id));
                rets.push_back({types[cti.id]});
                pt += 9;
            }
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
    if (t->tt == ettype::PFUNCTION) {
        mangled = mangle(t);
    } else if (t->tt == ettype::FUNCTION) {
        mangled = mangle(t);
        std::string pure_mangled = mangle_pure(t);
        type* ptp = get(pure_mangled);
        if (!ptp) {
            ptp = add_type(pure_mangled);
        }
        t->as_function().pure = &ptp->as_pfunction();
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

type* type_table::get(const std::string& mangled){
    auto t = mangle_table.find(mangled);
    if (t == mangle_table.end()) {
        return nullptr;
    } else {
        return types[t->second];
    }
}

type_id type_table::next_id() {
    return types.size();
}

