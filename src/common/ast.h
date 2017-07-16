#pragma once

#include <variant>
#include <vector>

#include "convenience.h"
#include "grammar.h"
#include "symbol_table.h"

struct ast;

enum class NodeType : u8 {
    NONE, SYMBOL,
    BYTE, WORD, DWORD, QWORD,
    STRING, ARRAY, STRUCT,
    PRE_UNARY, POST_UNARY, BINARY,
    BLOCK, FUNCTION
};

struct ast_node_none {};

struct ast_node_symbol {
    st_entry* symbol;
    std::string name;
};

struct ast_node_byte {
    u8 data;
    uid type;
};

struct ast_node_word {
    u16 data;
    uid type;
};

struct ast_node_dword {
    u32 data;
    uid type;
};

struct ast_node_qword {
    u64 data;
    uid type;
};

struct ast_node_string {
    u8* chars;
    u64 length;
    
    ~ast_node_string();
};

struct ast_node_array {
    ast** elements;
    u64 length;
    uid type;
    
    ~ast_node_array();
};

struct ast_node_struct {
    ast** elements;
    uid type;
    
    ~ast_node_struct();
};

struct ast_node_unary {
    Grammar::Symbol op;
    ast* node;
    uid type;
    bool assignable = false;
    
    ~ast_node_unary();
    void clean();
};

struct ast_node_binary {
    Grammar::Symbol op;
    ast* left;
    ast* right;
    uid type;
    bool assignable = false;
    
    ~ast_node_binary();
    void clean();
};

struct ast_node_block {
    std::vector<ast*> stmts;
    symbol_table* st = nullptr;
};

struct ast_node_function {
    uid type;
    ast* block;
};

using ast_node = std::variant
       <ast_node_none, ast_node_symbol,
        ast_node_byte, ast_node_word, ast_node_dword, ast_node_qword,
        ast_node_string, ast_node_array,
        ast_node_unary, ast_node_binary,
        ast_node_block, ast_node_function, ast_node_struct>;

struct ast {
    NodeType type;
    ast_node* node;

    ast();
    ast(ast_node_symbol node);
    ast(ast_node_byte node);
    ast(ast_node_word node);
    ast(ast_node_dword node);
    ast(ast_node_qword node);
    ast(ast_node_string node);
    ast(ast_node_array node);
    ast(ast_node_unary node, bool post = true);
    ast(ast_node_binary node);
    ast(ast_node_block node);
    ast(ast_node_function node);
    ast(ast_node_struct node);
    
    ast_node_none& get_none();
    ast_node_symbol& get_symbol();
    ast_node_byte& get_byte();
    ast_node_word& get_word();
    ast_node_dword& get_dword();
    ast_node_qword& get_qword();
    ast_node_string& get_string();
    ast_node_array& get_array();
    ast_node_unary& get_unary();
    ast_node_binary& get_binary();
    ast_node_block& get_block();
    ast_node_function& get_function();
    ast_node_struct& get_struct();
    
    uid get_type();
};