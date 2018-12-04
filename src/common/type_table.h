#pragma once

#include "common/convenience.h"
#include "common/type.h"

struct ast;

namespace mangle_bytes {
    constexpr char normal_start = '@';
    
    constexpr char naked_ptr_start = '*';
    constexpr char unique_ptr_start = '!';
    constexpr char shared_ptr_start = '+';
    constexpr char weak_ptr_start = '?';
    constexpr char array_ptr_start = '[';
    
    constexpr char struct_start = '{';
    constexpr char union_start = 'U';
    constexpr char struct_separator = '{';
    
    constexpr char function_start = ':';
    constexpr char function_return = ':';
    constexpr char function_param = ',';
    
    constexpr char combination_start = 'C';
};

class type_table {
public:
    type_table();
    ~type_table();
    
    std::string mangle(type* t);
    std::string mangle_pure(type* t);
    type unmangle(const std::string& mangled);
    
    type* add_type(type& t);
    type* add_type(type* t);
    type* add_type(const std::string& mangled);
    
    type* update_type(type_id id, type& nvalue);
    
    type* get(type_id id);
    type* get(type& t);
    type* get(const std::string& mangled);
    
    type* get_or_add(type& t);
    
    type_id next_id();
    
    void merge(type_table&& o);
    
    std::string print();
    
    static type* t_void;
    static type* t_byte;
    static type* t_short;
    static type* t_int;
    static type* t_long;
    static type* t_sig;
    static type* t_float;
    static type* t_double;
    static type* t_bool;
    static type* t_char;
    static type* t_string;
    static type* t_fun;
    static type* t_let;
    static type* t_null;
    static type* t_nothing;
private:
    std::vector<type*> types{};
    dict<type_id> mangle_table{};
};
