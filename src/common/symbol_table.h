#pragma once

#include <utility>
#include <vector>

#include "common/defs.h"

struct type;
struct ast;
class symbol_table;
struct symbol;
class nnmodule;

enum class symbol_type {
    VARIABLE, OVERLOAD, NAMESPACE, MODULE, LABEL
};

struct symbol_variable {
    type* t;
    ast* value;
    symbol_table* st;
    
    bool defined : 1;
    bool compiletime : 1;
    bool modified : 1;
    bool used : 1;
}; 

struct symbol_overload {
    std::vector<symbol*> syms;
};

struct symbol_namespace {
    symbol_table* st;
};

struct symbol_module {
    nnmodule* mod;
};

struct symbol_label {
    
};

struct symbol {
    symbol_type tt;
    std::string name;
    symbol_table* owner;
    
    union {
        symbol_variable variable;
        symbol_overload overload;
        symbol_namespace namespace_;
        symbol_module mod;
        symbol_label label;
    };
    
    symbol();
    symbol(const symbol& o);
    ~symbol();
    symbol(symbol_type tt, const std::string& name, symbol_table* owner);
    symbol(const std::string& name, const symbol_variable& variable);
    symbol(const std::string& name, const symbol_overload& overload);
    symbol(const std::string& name, const symbol_namespace& ns);
    symbol(const std::string& name, const symbol_module& mod);
    symbol(const std::string& name, const symbol_label& label);
    
    bool is_variable();
    bool is_overload();
    bool is_namespace();
    bool is_module();
    bool is_label();
};

class symbol_table {
public:
    symbol_table();
    ~symbol_table();
    
    symbol_table* make_child();
    
    symbol* get(const std::string& name);
    std::pair<symbol_table*, symbol*> find(const std::string& name);
    
    symbol* add(const std::string& name, symbol* sym);
    symbol* add_anonymous(symbol* sym);
    symbol* add_borrowed(const std::string& name, symbol* sym);
    
    symbol* add_undefined(const std::string& name, type* t); // HAS to be compiletime
    symbol* add_primitive(const std::string& name, type* t, ast* value, bool defined = true, bool compiletime = false);
    symbol* add_type(const std::string& name, type* t, ast* value, symbol_table* st);
//     symbol* add_function(const std::string& name, type* t, ast* value, symbol_table* st); // Overloadable
    symbol* add_namespace(const std::string& name);
    symbol* add_module(const std::string& name, nnmodule* mod);
    symbol* add_label(const std::string& name);
    
    symbol* make_overload(const std::string& name);
    
    // Returns first offending element
    symbol* borrow(symbol_table* o);
    
private:
    symbol_table* parent{nullptr};
    symbol* owner{nullptr};
    
    std::vector<symbol*> anonymous{};
    dict<std::string, symbol*> symbols{};
    dict<std::string, symbol*> borrowed{};
    
    std::vector<symbol_table*> children{};
};
