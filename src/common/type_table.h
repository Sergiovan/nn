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
    
    type* get(type_id id);
    type* get(type& t);
    type* get(const std::string& mangled);
    
    type_id next_id();
    
    void merge(type_table&& o);
    
    type* t_void{nullptr};
    type* t_byte{nullptr};
    type* t_short{nullptr};
    type* t_int{nullptr};
    type* t_long{nullptr};
    type* t_sig{nullptr};
    type* t_float{nullptr};
    type* t_double{nullptr};
    type* t_bool{nullptr};
    type* t_char{nullptr};
    type* t_string{nullptr};
    type* t_fun{nullptr};
    type* t_let{nullptr};
    type* t_null{nullptr};
    type* t_nothing{nullptr};
private:
    std::vector<type*> types{};
    dict<type_id> mangle_table{};
};
