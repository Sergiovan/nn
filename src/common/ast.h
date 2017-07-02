#pragma once

#include <variant>
#include <vector>

#include "convenience.h"
#include "value.h"
#include "grammar.h"

struct ast;

enum class NodeType : u8 {
    NONE, VALUE,
    PRE_UNARY, POST_UNARY, BINARY,
    BLOCK, FUNCTION
};

struct ast_node_none {};

struct ast_node_value {
    value val;
};

struct ast_node_unary {
    Grammar::Symbol op;
    ast* node;
    
    ~ast_node_unary();
    void clean();
};

struct ast_node_binary {
    Grammar::Symbol op;
    ast* left;
    ast* right;
    
    ~ast_node_binary();
    void clean();
};

struct ast_node_block {
    std::vector<ast*> stmts;
};

struct function_param_opts {
    std::string name;
    bool def_val = false;
    value val;
};

struct ast_node_function {
    u64 type;
    std::vector<function_param_opts> param_opts;
};

using ast_node = std::variant
    <ast_node_none, ast_node_value, ast_node_unary, ast_node_binary, ast_node_block, ast_node_function>;

struct ast {
    NodeType type;
    ast_node node;
};
