#include "type_table.h"

type_table::type_table() {
    /* Add all required default types here */
    add_type(type{TypeType::PRIMITIVE, type_primitive{PrimitiveType::VOID}}); // Void
    
    add_type(type{TypeType::PRIMITIVE, type_primitive{PrimitiveType::BYTE}}); // Byte
    add_type(type{TypeType::PRIMITIVE, type_primitive{PrimitiveType::SHORT}}); // Short
    add_type(type{TypeType::PRIMITIVE, type_primitive{PrimitiveType::INT}}); // Int
    add_type(type{TypeType::PRIMITIVE, type_primitive{PrimitiveType::LONG}}); // Long
    
    add_type(type{TypeType::PRIMITIVE, type_primitive{PrimitiveType::FLOAT}}); // Float
    add_type(type{TypeType::PRIMITIVE, type_primitive{PrimitiveType::DOUBLE}}); // Double
    add_type(type{TypeType::PRIMITIVE, type_primitive{PrimitiveType::__LDOUBLE}}); // Long doube
    
    add_type(type{TypeType::PRIMITIVE, type_primitive{PrimitiveType::CHAR}}); // Char
    add_type(type{TypeType::PRIMITIVE, type_primitive{PrimitiveType::STRING}}); // String
    
    add_type(type{TypeType::PRIMITIVE, type_primitive{PrimitiveType::BOOL}}); // Bool
    add_type(type{TypeType::PRIMITIVE, type_primitive{PrimitiveType::SIG}}); // Sig
    add_type(type{TypeType::PRIMITIVE, type_primitive{PrimitiveType::FUN}}); // Fun
    add_type(type{TypeType::PRIMITIVE, type_primitive{PrimitiveType::LET}}); // Let
}

uid type_table::add_type(type t) {
    types.push_back(t);
    return types.size() - 1;
}

type& type_table::get_type(uid id) {
    return types[id];
}

type& type_table::operator[](uid id) {
    return get_type(id);
}