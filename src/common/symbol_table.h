#pragma once

#include <variant>
#include <vector>

#include "common/convenience.h"

class symbol_table;
struct ast;
struct type;

enum class etable_owner {
    FREE, BLOCK, NAMESPACE, LOOP, 
    FUNCTION, STRUCT, MODULE,
    UNION, ENUM,
    COPY
};

enum class est_entry_type {
    TYPE, VARIABLE, FUNCTION, NAMESPACE,
    FIELD, OVERLOAD, MODULE, LABEL
};

struct overload {
    type* t;
    ast* value{nullptr}; // Owned
    bool defined{false};
    symbol_table* st{nullptr}; // Owned
    u64 oid{0};
    
    overload(type* t, ast* value = nullptr, bool defined = false, symbol_table* st = nullptr, u64 oid = 0);
    ~overload();
    overload(const overload& o);
    overload(overload&& o);
    overload& operator=(const overload& o);
    overload& operator=(overload&& o);
    
    std::string unique_name();
};

struct st_type {
    type* t;
    bool defined{false};
    symbol_table* st{nullptr}; // Owned
    
    st_type(type* t, bool defined = false, symbol_table* st = nullptr);
    ~st_type();
    st_type(const st_type& o);
    st_type(st_type&& o);
    st_type& operator=(const st_type& o);
    st_type& operator=(st_type&& o);
};

struct st_variable {
    type* t;
    ast* value{nullptr}; // Owned
    bool defined{false};
    
    st_variable(type* t, ast* value = nullptr, bool defined = false);
    ~st_variable();
    st_variable(const st_variable& o);
    st_variable(st_variable&& o);
    st_variable& operator=(const st_variable& o);
    st_variable& operator=(st_variable&& o);
};

struct st_function {
    std::vector<overload*> overloads{}; // Owned
    // TODO Borrowed overloads, for inner functions with the same name as outer functions
    symbol_table* st{nullptr}; // Owned, for sigs and overloads
    
    overload* get_overload(const std::vector<type*>& args);
    
    st_function(const std::vector<overload*> overloads = {}, symbol_table* st = nullptr);
    ~st_function();
    st_function(const st_function& o);
    st_function(st_function&& o);
    st_function& operator=(const st_function& o);
    st_function& operator=(st_function&& o);
};

struct st_namespace {
    symbol_table* st{nullptr}; // Owned
    
    st_namespace(symbol_table* st = nullptr);
    ~st_namespace();
    st_namespace(const st_namespace& o);
    st_namespace(st_namespace&& o);
    st_namespace& operator=(const st_namespace& o);
    st_namespace& operator=(st_namespace&& o);
};

struct st_field {
    u64 field;
    type* ptype{nullptr};
};

struct st_overload {
    overload* ol{nullptr}; // Not owned
    st_function* function{nullptr}; // Not owned
};

struct st_label {};

using st_variant = std::variant<st_type, st_variable, st_function, st_namespace, st_field, st_overload, st_label>;

struct st_entry {
    st_variant entry;
    est_entry_type t;
    
    std::string name{};
    
    static st_entry* variable(const std::string& name, type* t, ast* value = nullptr, bool defined = false);
    static st_entry* field(const std::string& name, u64 field, type* ptype = nullptr);
    
    st_type& as_type();
    st_variable& as_variable();
    st_function& as_function();
    st_namespace& as_namespace();
    st_field& as_field();
    st_overload& as_overload();
    st_namespace& as_module();
    
    
    bool is_type();
    bool is_variable();
    bool is_function();
    bool is_namespace();
    bool is_field();
    bool is_module();
    bool is_overload();
    bool is_label();
    
    type* get_type();
    
    std::string print(u64 depth = 0);
};

class symbol_table {
public:
    symbol_table(etable_owner owner, symbol_table* parent = nullptr);
    
    ~symbol_table();
    symbol_table(const symbol_table& o);
    symbol_table(symbol_table&& o);
    symbol_table& operator=(const symbol_table& o);
    symbol_table& operator=(symbol_table&& o);
    
    bool has(const std::string& name, bool propagate = true, etable_owner until = etable_owner::FREE);
    
    st_entry* get(const std::string& name, bool propagate = true, etable_owner until = etable_owner::FREE);
    st_entry* get(const std::string& name, est_entry_type t, bool propagate = true, etable_owner until = etable_owner::FREE);
    overload* get_overload(const std::string& name, std::vector<type*>& args, bool propagate = true, etable_owner until = etable_owner::FREE);
    overload* get_overload(const std::string& name, type* ftype, bool propagate = true, etable_owner until = etable_owner::FREE);
    
    st_entry* add(const std::string& name, st_entry* entry);
    st_entry* add_type(const std::string& name, type* t, bool defined = true);
    st_entry* add_variable(const std::string& name, type* t, ast* value = nullptr);
    st_entry* add_or_get_empty_function(const std::string& name);
    std::pair<st_entry*, overload*> add_function(const std::string& name, type* function, ast* value = nullptr, symbol_table* st = nullptr);
    st_entry* add_namespace(const std::string& name, symbol_table* st = nullptr);
    st_entry* add_module(const std::string& name, symbol_table* st);
    st_entry* add_field(const std::string& name, u64 field, type* ptype);
    st_entry* add_label(const std::string& name);
    
    st_entry* borrow(const std::string& name, st_entry* entry);
    bool merge_st(symbol_table* st); // TODO Errors and merge conflicts
    
    u64 get_size(bool borrowed = true);
    
    void set_owner(etable_owner owner);
    bool owned_by(etable_owner owner);
    
    symbol_table* make_child(etable_owner new_owner = etable_owner::COPY);
    
    std::string print(u64 depth = 0);

    symbol_table* parent;
    etable_owner owner;
    dict<st_entry*> entries{}; // Owned
    dict<st_entry*> borrowed_entries{}; // NOT Owned
};

std::ostream& operator<<(std::ostream& os, const est_entry_type& st_entry_type);
