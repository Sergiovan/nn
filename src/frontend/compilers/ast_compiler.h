#pragma once

#include <string>

class compiler;
class nnmodule;
struct ast;
struct type;
class type_table;
struct symbol;
class symbol_table;

class ast_compiler {
public:
    ast_compiler(compiler& c, nnmodule& mod, ast* node, symbol_table* st, symbol* sym);
    
    void compile_root();
private:
    
    using comp_func = void (ast_compiler::*)(ast*);
    
    void compile_def(ast* root);
    void compile_block(ast* root);
    void compile_block(ast* root, comp_func f);
    
    void compile(ast* node);
    void compile_zero(ast* node);
    void compile_unary(ast* node);
    void compile_binary(ast* node);
    void compile_compound(ast* node);
    void compile_struct(ast* node);
    void compile_enum(ast* node);
    void compile_tuple(ast* node);
    void compile_function(ast* node);
    
    ast* get_compiletime_value(ast* node);
    
    void size_loop(type* t);
    void define_loop(symbol* sym);
    
    std::string conversion_error(type* from, type* to);
    
    compiler& comp;
    nnmodule& mod;
    
    ast* root_node;
    
    type_table& tt;
    symbol_table& mod_st;
    symbol_table& root_st;
    symbol* root_sym;
    
    ast* block_node;
};
