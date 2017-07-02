#pragma once

#include <variant>
#include <vector>

#include "convenience.h"
#include "symbol_table.h"
#include "value.h"

enum class TypeType : u8 {
    POINTER, FUNCTION, STRUCT_OR_UNION, ENUM, ARRAY, PRIMITIVE
};

enum class PointerType {
    NAKED, SHARED, WEAK, UNIQUE
};

enum class TypeFlag : u8 {
    VOLATILE = 1 << 0,
    CONST    = 1 << 1
};

struct type_ptr {
    u64 at{0};
    PointerType type{PointerType::NAKED};
};

struct type_func {
    std::vector<u64> returns{};
    std::vector<u64> args{};
};

struct type_struct_union {
    bool union_type = false;
    std::vector<value> elems{};
    symbol_table* elem_names = nullptr;
    
    type_struct_union();
    ~type_struct_union();
};

struct type_enum {
    trie<u64>* enum_names = nullptr;
    
    type_enum();
    ~type_enum();
};

struct type_array {
    u64 of{0};
};

using type_union = std::variant<type_ptr, type_func, type_struct_union, type_enum, type_array>;

struct type {
    u8 flags;
    TypeType type;
    type_union value;
};

class type_table {
public:
    type_table();
    
    u64 add_type(type& t);
private:
    std::vector<type> types{};
};
