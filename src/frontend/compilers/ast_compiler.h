#pragma once

class compiler;
class nnmodule;
struct ast;
struct type;
class type_table;
struct symbol;
class symbol_table;

class ast_compiler {
public:
    ast_compiler(compiler& c, nnmodule& mod, ast* node);
    
    void compile_root(ast* root, symbol_table* st, symbol* sym);
private:
    
    using comp_func = void (ast_compiler::*)(ast*, symbol_table*, symbol*);
    
    void compile_def(ast* root, symbol_table* st, symbol* sym);
    void compile_block(ast* root, symbol_table* st, symbol* sym);
    void compile_block(ast* root, symbol_table* st, symbol* sym, comp_func f);
    
    void compile(ast* node, symbol_table* st, symbol* sym);
    void compile_zero(ast* node, symbol_table* st, symbol* sym);
    void compile_unary(ast* node, symbol_table* st, symbol* sym);
    void compile_binary(ast* node, symbol_table* st, symbol* sym);
    void compile_compound(ast* node, symbol_table* st, symbol* sym);
    void compile_struct(ast* node, symbol_table* st, symbol* sym);
    void compile_enum(ast* node, symbol_table* st, symbol* sym);
    void compile_tuple(ast* node, symbol_table* st, symbol* sym);
    void compile_function(ast* node, symbol_table* st, symbol* sym);
    
    ast* get_compiletime_value(ast* node, symbol_table* st, symbol* sym);
    
    void size_loop(type* t);
    void define_loop(symbol* sym);
    
    compiler& comp;
    nnmodule& mod;
    
    ast* root_node;
    
    type_table& tt;
    symbol_table& root_st;
};
