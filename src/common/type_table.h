#pragma once

#include <variant>
#include <vector>
#include <utility>

#include "convenience.h"
#include "symbol_table.h"
#include "value.h"

enum class TypeType : u8 {
    POINTER, FUNCTION, STRUCT_OR_UNION,
    ENUM, ARRAY, PRIMITIVE
};

enum class PrimitiveType : u8 {
    INTEGER, REAL, BOOLEAN, CHARACTER,
    STRING, VOID
};

enum class PointerType {
    NAKED, SHARED, WEAK, UNIQUE
};

struct type_ptr {
    u64 at{0};
    PointerType type{PointerType::NAKED};
};

struct type_func {
    std::vector<std::pair<u64, u8>> returns{};
    std::vector<std::pair<u64, u8>> args{};
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

struct type_primitive {
    u8 size;
    PrimitiveType type;
};

using type_union = std::variant
    <type_ptr, type_func, type_struct_union, type_enum, type_array, type_primitive>;

struct type {
    TypeType type;
    type_union value;
};

class type_table {
public:
    type_table();
    
    u64 add_type(type t);
private:
    std::vector<type> types{};
};
