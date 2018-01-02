#include "type_table.h"
#include <cstring>

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

uid type_table::add_type(std::string& mangled) {
    size_t len = mangled.length() + 2;
    char* s = new char[len];
    std::memcpy(s+1, &mangled[0], sizeof(char) + mangled.length());
    s[0] = s[len - 1] = '\0';
    if(mangled[0] == ':') { // Function
        type t{TypeType::FUNCTION, type_func{{}, {}, {}}};
        auto& rets = t.get_func().returns;
        auto& args = t.get_func().args;
        char b[15]; // 2147483647, segfaulty
        u8 varclass;
        bool isargs = false, isvarclass = true;
        std::memset(b, 0, sizeof b);
        u8 i = 0;
        for(char* c = s + 2; c; c++) { // Skip first :
            if(isvarclass) {
                isvarclass = false;
                varclass = (u8) *c;
            }
            if(*c >= '0' && *c <= '9') {
                b[i++] = *c;
            } else {
                if(!isargs) {
                    rets.push_back({(uid) std::stoi(b), varclass});
                    if(*c == ',') {
                        isargs = true;
                    }
                } else {
                    args.push_back({{(uid) std::stoi(b), varclass}});
                }
                i = 0;
                std::memset(b, 0, sizeof b);
                isvarclass = true;
            }
        }

        return add_type(t);
    } else {
        for(char* c = s + (len - 2); c; c--) {

        }
    }
}

type& type_table::get_type(uid id) {
    return types[id];
}

type& type_table::operator[](uid id) {
    return get_type(id);
}