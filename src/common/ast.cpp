#include "ast.h"

ast_node_unary::~ast_node_unary() {
    delete node;
}

void ast_node_unary::clean() {
    node = nullptr;
}

ast_node_binary::~ast_node_binary() {
    delete left;
    delete right;
}

void ast_node_binary::clean() {
    left = nullptr;
    right = nullptr;
}

