#pragma once

#include <string>
#include <variant>
#include <vector>
#include <utility>

#include "value.h"

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
    types_union value;
};