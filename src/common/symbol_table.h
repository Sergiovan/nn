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
    type* t; // Variable type
    ast* value; // Variable default value
    symbol_table* st; // Variable inner symbol table
    // For functions: Holds named e64 values and labels
    
    bool defined; // If the variable has been defined yet
    bool compiletime; // If the variable is always available at compiletime
    bool reference; // If the variable is a reference
    bool modified {false}; // If the value of this variable has been modified after initialization
    bool used {false}; // If the value of this variable has been read or written to after initialization
    bool thisarg {false}; // If this is this
    bool member {false}; // If this is a member variable
    
    // Functions only
    bool infer_ret {false}; // If the return is entirely inferred
    bool compiletime_call{false}; // If the function can be _called_ at compiletime (All functions are available at compiletime, as they're simply types)
    // bool add_e64 {false}; // If the return needs to have an e64 added // NOTE Removed, all errors explicit or inferred
    ast* gotos{nullptr}; // Stores gotos in the function that have not yet been handled. Temporary
    
    // Returns
    bool is_return {false}; // If this is a return variable name
    
    u64 order{0};
}; 

struct symbol_overload {
    std::vector<symbol*> syms;
};

// TODO Merge into variables lazily
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
    ast* decl;
    
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
    symbol(symbol_type tt, const std::string& name, symbol_table* owner, ast* decl);
    symbol(const std::string& name, ast* decl, const symbol_variable& variable);
    symbol(const std::string& name, ast* decl, const symbol_overload& overload);
    symbol(const std::string& name, ast* decl, const symbol_namespace& ns);
    symbol(const std::string& name, ast* decl, const symbol_module& mod);
    symbol(const std::string& name, ast* decl, const symbol_label& label);
    
    bool is_variable();
    bool is_overload();
    bool is_namespace();
    bool is_module();
    bool is_label();
};

class symbol_table {
public:
    symbol_table(symbol* owner = nullptr);
    ~symbol_table();
    
    symbol_table* make_child(symbol* owner = nullptr);
    
    // Only searches this st
    symbol* get(const std::string& name);
    // Searches parent sts too
    std::pair<symbol_table*, symbol*> find(const std::string& name);
    
    symbol* add(const std::string& name, symbol* sym);
    symbol* add_anonymous(symbol* sym);
    symbol* add_borrowed(const std::string& name, symbol* sym);
    
    symbol* add_unnamed(type* t, ast* decl, bool defined = true, bool compiletime = false, 
                        bool reference = false, 
                        bool thisarg = false, bool member = false); // For counting purposes
    
    symbol* add_undefined(const std::string& name, type* t, ast* decl); // HAS to be compiletime
    symbol* make_and_add_placeholder(const std::string& name, type* t, ast* decl); // Also has to be compiletime
    symbol* add_primitive(const std::string& name, type* t, ast* decl, ast* value, bool defined = true, 
                          bool compiletime = false, bool reference = false, bool thisarg = false, 
                          bool member = false);
    symbol* add_type(const std::string& name, type* t, ast* decl, ast* value, symbol_table* st);
//     symbol* add_function(const std::string& name, type* t, ast* value, symbol_table* st); // Overloadable
    symbol* add_namespace(const std::string& name, ast* decl);
    symbol* add_module(const std::string& name, ast* decl, nnmodule* mod);
    symbol* add_label(const std::string& name, ast* decl);
    
    symbol* make_overload(const std::string& name);
    
    // Returns first offending element
    symbol* borrow(symbol_table* o);
    
    symbol* get_owner();
    
private:
    symbol_table* parent{nullptr};
    symbol* owner{nullptr};
    
    std::vector<symbol*> anonymous{};
    dict<std::string, symbol*> symbols{};
    dict<std::string, symbol*> borrowed{};
    
    std::vector<symbol_table*> children{};
};
