#pragma once

#include <vector>

#include "common/defs.h"
#include "common/list.h"

struct ast;
class type_table;
class symbol_table;

enum class type_type : u8 {
    PRIMITIVE, POINTER, 
    ARRAY, COMPOUND,
    
    STRUCT, UNION, ENUM, TUPLE, 
    FUNCTION, SUPERFUNCTION,
    
    SPECIAL
};

enum class primitive_type : u8 {
    SIGNED, UNSIGNED, BOOLEAN, FLOATING, 
    CHARACTER, ERROR, TYPE, ANY, VOID
};

enum class pointer_type : u8 {
    NAKED, UNIQUE, SHARED, WEAK
};

enum class special_type : u8 {
    INFER, // Becomes whatever type is assigned to it
    GENERIC, GENERIC_UNKNOWN, // Selected at compiletime. GENERIC_UNKNOWN is the result of operations that use GENERIC inside a generic function or type
    GENERIC_COMPOUND, // GENERIC_COMPOUND is the result of spreading a generic
    NOTHING, // Does not initialize to 0
    TYPELESS, // Has no type
    NONE, // Has undecided type
    NONE_ARRAY, NONE_STRUCT, NONE_TUPLE,
    NONE_FUNCTION, 
    NULL_, // Null pointer type
    ERROR_TYPE, // Type error
    ERROR_COMPOUND, // Type error. ERROR_COMPOUND is the result of spreading a type error
};

struct type;

struct type_primitive {
    primitive_type tt;
    u16 bits;
};

struct type_pointer {
    pointer_type tt;
    type* at;
};

struct type_array {
    type* at;
    bool sized;
    u64 size;
};

struct member {
    type* t;
    u8 compiletime : 1;
    u8 reference : 1;
};

struct type_compound {
    std::vector<member> members;
};

struct type_supercompound {
    type* comp;
    bool generic {false}; // Any members are generic
    bool generated {false}; // Compound type was generated at compiletime by instantiating generic compound
    symbol_table* st{nullptr}; // Copy of variable st, for convenience
};

struct param {
    type* t;
    u8 compiletime : 1;
    u8 reference : 1;
    u8 spread : 1;
    u8 generic : 1;
    u8 binding : 1;
    u8 thisarg : 1;
};

struct ret {
    type* t;
    u8 compiletime : 1;
    u8 reference : 1;
};

// Names and default values are __not__ part of the function type
struct type_function {
    std::vector<param> params{};
    std::vector<ret> rets{};
};

struct param_info {
    std::string name{};
    bool defaulted {false};
    ast* value{nullptr};
};

struct ret_info {
    std::string name;
};

struct type_superfunction { // TODO Add extra data, like captures and stuff
    type* function;
    std::vector<param_info> params{};
    std::vector<ret_info> rets{};
    bool generic{false}; // Any parameters are generic
    bool generated{false}; // Function type was created at compile-time by calling generic function
    symbol_table* st{nullptr}; // Copy of variable st, for convenience
};

struct type_special {
    special_type tt;
};

struct type {
    type_table& table;
    u64 id{0xFFFFFFFF};
    type_type tt{type_type::SPECIAL};
    
    u8 const_flag : 1;
    u8 volat_flag : 1;
    u8 sized : 1;
    
    u64 size{0};
    
    
    union {
        type_primitive primitive;
        type_pointer pointer;
        type_array array;
        type_compound compound;
        type_supercompound scompound;
        type_function function;
        type_superfunction sfunction;
        type_special special{special_type::NONE};
    };
    
    bool is_primitive();
    bool is_primitive(primitive_type tt);
    bool is_pointer();
    bool is_pointer(pointer_type tt);
    bool is_array();
    bool is_array(bool sized);
    bool is_compound();
    bool is_supercompound();
    bool is_struct();
    bool is_union();
    bool is_enum();
    bool is_enum(primitive_type tt);
    bool is_tuple();
    bool is_function();
    bool is_superfunction();
    bool is_supertype();
    bool is_special();
    bool is_special(special_type tt);
    bool is_generic();
    
    type* get_underlying();
    
    bool set_size();
    
    type(type_table& tt, u64 id, const type_primitive& primitive, bool _const, bool volat);
    type(type_table& tt, u64 id, const type_pointer& pointer, bool _const, bool volat);
    type(type_table& tt, u64 id, const type_array& array, bool _const, bool volat);
    type(type_table& tt, u64 id, const type_compound& compound, bool _const, bool volat);
    type(type_table& tt, u64 id, type_type ttt, const type_supercompound& scompound, bool _const, bool volat);
    type(type_table& tt, u64 id, const type_function& function, bool _const, bool volat);
    type(type_table& tt, u64 id, const type_superfunction& sfunction, bool _const, bool volat);
    type(type_table& tt, u64 id, const type_special& special, bool _const, bool volat);
    
    ~type();
    type(type&& o);
    
    std::string to_string(bool simple = false);
};

std::ostream& operator<<(std::ostream& os, type_type t);
std::ostream& operator<<(std::ostream& os, primitive_type t);
std::ostream& operator<<(std::ostream& os, pointer_type t);
std::ostream& operator<<(std::ostream& os, special_type t);
