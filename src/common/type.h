#pragma once

#include <string>
#include <variant>
#include <vector>
#include <utility>

#include "value.h"
#include "convenience.h"

struct ast_node_function;

struct ptype {
    uid type{0};
    vflags flags{0};
};

struct field {
    ptype type;
};

struct sfield {
    ptype type;
    value data{nullptr}; // TODO It's a pointer
    u8 bits{64};
    bool is_bitfield = false;
};

struct overload {
    ptype type;
    ast_node_function* function{nullptr};
    bool generic{false};
};

struct parameter {
    ptype type;
    std::string name{""};
    value data = nullptr; // TODO It's a pointer
    bool var_length = false;
};

enum class TypeType : u8 {
    PRIMITIVE, POINTER, STRUCT,
    UNION, ENUM, FUNCTION
};

enum class PrimitiveType : u8 {
    VOID, BYTE, SHORT, INT, LONG, FLOAT, DOUBLE,
    __LDOUBLE, CHAR, STRING, BOOL, SIG, FUN, LET
};

namespace TypeID {
    constexpr uid VOID = 0;
    
    constexpr uid BYTE = 1;
    constexpr uid SHORT = 2;
    constexpr uid INT = 3;
    constexpr uid LONG = 4;
    
    constexpr uid FLOAT = 5;
    constexpr uid DOUBLE = 6;
    constexpr uid __LDOUBLE = 7;
    
    constexpr uid CHAR = 8;
    constexpr uid STRING = 9;
    
    constexpr uid BOOL = 10;
    
    constexpr uid SIG = 11;
    
    constexpr uid FUN = 12;
    constexpr uid LET = 13;
}

enum class PointerType {
    NAKED, SHARED, WEAK, UNIQUE
};

struct type_primitive {
    PrimitiveType type{PrimitiveType::VOID};
};

struct type_ptr {
    PointerType type{PointerType::NAKED};
    vflags flags{0};
    ptype at;
};

struct type_struct {
    std::vector<sfield> fields;
    trie<u64> field_names;
    std::string name{""};
};

struct type_union {
    std::vector<field> fields;
    trie<u64> field_names;
    std::string name{""};
};

struct type_enum {
    trie<u64> enum_names;
    std::string name{""};
};

struct type_func {
    std::vector<ptype> returns{};
    std::vector<parameter> args{};
    std::string name{""};
};

using types_union = std::variant
    <type_primitive, type_ptr, type_struct, type_union, type_enum, type_func>;

struct type {
    TypeType type;
    types_union data;
    
    type_ptr& get_ptr();
    type_func& get_func();
    type_struct& get_struct();
    type_union& get_union();
    type_enum& get_enum();
    type_primitive& get_primitive();
};