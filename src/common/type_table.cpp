#include "type_table.h"

type_struct_union::type_struct_union() {
    elem_names = new symbol_table;
}

type_struct_union::~type_struct_union() {
    delete elem_names;
}

type_enum::type_enum() {
    enum_names = new trie<u64>;
}

type_enum::~type_enum() {
    delete enum_names;
}

type_table::type_table() {
    /* Add all required default types here */
    add_type(type{TypeType::PRIMITIVE, type_primitive{0, PrimitiveType::VOID}}); // Void
    
    add_type(type{TypeType::PRIMITIVE, type_primitive{1, PrimitiveType::INTEGER}}); // Byte
    add_type(type{TypeType::PRIMITIVE, type_primitive{2, PrimitiveType::INTEGER}}); // Short
    add_type(type{TypeType::PRIMITIVE, type_primitive{4, PrimitiveType::INTEGER}}); // Int
    add_type(type{TypeType::PRIMITIVE, type_primitive{8, PrimitiveType::INTEGER}}); // Long
    
    add_type(type{TypeType::PRIMITIVE, type_primitive{4, PrimitiveType::REAL}}); // Float
    add_type(type{TypeType::PRIMITIVE, type_primitive{8, PrimitiveType::REAL}}); // Float
    
    add_type(type{TypeType::PRIMITIVE, type_primitive{4, PrimitiveType::CHARACTER}}); // Char
    add_type(type{TypeType::PRIMITIVE, type_primitive{8, PrimitiveType::STRING}}); // String
    
    add_type(type{TypeType::PRIMITIVE, type_primitive{1, PrimitiveType::BOOLEAN}}); // Bool
}

u64 type_table::add_type(type t) {
    types.push_back(t);
    return types.size() - 1;
}