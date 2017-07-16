#pragma once

#include <string>
#include <variant>
#include <vector>
#include <utility>

#include "value.h"
#include "trie.h"

struct ast_node_function;

struct field {
    uid type;
    std::string name;
    value data;
    vflags flags;
    bool has_value = false, is_bitfield = false;
};

struct ufield {
    uid type;
    std::string name;
    vflags flags;
};

struct ffield {
    uid type;
    std::string name;
    value data;
    vflags flags;
    bool has_name = true, has_value = false;
};

struct nnreturn {
    uid type;
    vflags flags;
};

struct overload {
    uid type;
    bool defined;
    ast_node_function* function;
};

enum class TypeType : u8 {
    POINTER, FUNCTION, STRUCT, UNION,
    ENUM, ARRAY, PRIMITIVE
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

struct type_ptr {
    PointerType type{PointerType::NAKED};
    uid at{0};
    vflags flags; // Flags of what is being pointed at
};

struct type_func {
    std::vector<nnreturn> returns{};
    std::vector<ffield> args{};
};

struct type_struct {
    std::vector<field> fields;
};

struct type_union {
    std::vector<ufield> fields;
};

struct type_enum {
    trie<u16>* enum_names = nullptr;
    
    type_enum();
    ~type_enum();
};

struct type_array {
    uid of{0};
};

struct type_primitive {
    PrimitiveType type;
};

using types_union = std::variant
    <type_ptr, type_func, type_struct, type_union, type_enum, type_array, type_primitive>;

struct type {
    TypeType type;
    types_union data;
    
    type_ptr& get_ptr();
    type_func& get_func();
    type_struct& get_struct();
    type_union& get_union();
    type_enum& get_enum();
    type_array& get_array();
    type_primitive& get_primitive();
};