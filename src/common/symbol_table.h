#pragma once

#include <variant>
#include <vector>

#include "common/convenience.h"

class symbol_table;
struct ast;
struct type;

enum class etable_owner {
    FREE, BLOCK, NAMESPACE, FUNCTION, STRUCT, MODULE
};

enum class est_entry_type {
    TYPE, VARIABLE, FUNCTION, NAMESPACE,
    FIELD, MODULE
};

struct overload {
    type* t;
    ast* value{nullptr}; // Owned
    bool defined{false};
    symbol_table* st{nullptr}; // Owned
    
    ~overload();
};

struct st_type {
    type* t;
    bool defined{false};
    symbol_table* st{nullptr}; // Owned
    
    ~st_type();
};

struct st_variable {
    type* t;
    ast* value{nullptr}; // Owned
    bool defined{false};
    
    ~st_variable();
};

struct st_function {
    std::vector<overload> overloads{};
};

struct st_namespace {
    symbol_table* st{nullptr}; // Owned
    
    ~st_namespace();
};

struct st_field {
    u64 field;
};

using st_variant = std::variant<st_type, st_variable, st_function, st_namespace, st_field>;

struct st_entry {
    st_variant entry;
    est_entry_type t;
    
    st_type& as_type();
    st_variable& as_variable();
    st_function& as_function();
    st_namespace& as_namespace();
    st_field& as_field();
    
    bool is_type();
    bool is_variable();
    bool is_function();
    bool is_namespace();
    bool is_field();
    bool is_module();
};

class symbol_table {
public:
    symbol_table(etable_owner owner, symbol_table* parent = nullptr);
    ~symbol_table();
    
    bool has(const std::string& name, bool propagate = true, etable_owner until = etable_owner::FREE);
    
    st_entry* get(const std::string& name, bool propagate = true, etable_owner until = etable_owner::FREE);
    st_entry* get(const std::string& name, est_entry_type t, bool propagate = true, etable_owner until = etable_owner::FREE);
    overload* get_overload(const std::string& name, std::vector<type*>& params, bool propagate = true, etable_owner until = etable_owner::FREE);
    
    st_entry* add(const std::string& name, st_entry* entry);
    st_entry* add_type(const std::string& name, type* t, bool defined = true);
    st_entry* add_variable(const std::string& name, type* t, ast* value = nullptr);
    std::pair<st_entry*, overload*> add_function(const std::string& name, type* function, ast* value = nullptr);
    st_entry* add_namespace(const std::string& name, symbol_table* st = nullptr);
    st_entry* add_module(const std::string& name, symbol_table* st);
    st_entry* add_field(const std::string& name, u64 field);
    
    bool merge_st(symbol_table* st);
    
private:
    symbol_table* parent;
    etable_owner owner;
    dict<st_entry*> entries{}; // Owned
    dict<st_entry*> borrowed_entries{}; // NOT Owned
};
