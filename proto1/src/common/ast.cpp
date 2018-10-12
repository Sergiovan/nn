#include "ast.h"
#include "type.h"

ast_node_string::~ast_node_string() {
    delete chars;
}

ast_node_array::~ast_node_array() {
    delete elements;
}

ast_node_struct::~ast_node_struct() {
    delete elements;
}

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

ast::ast() : type(NodeType::NONE), node(new ast_node(ast_node_none{})) {

}

ast::ast(ast_node_symbol node) : type(NodeType::SYMBOL), node(new ast_node(node)) {
}

ast::ast(ast_node_byte node) : type(NodeType::BYTE), node(new ast_node(node)) {

}

ast::ast(ast_node_word node) : type(NodeType::WORD), node(new ast_node(node)) {

}

ast::ast(ast_node_dword node) : type(NodeType::DWORD), node(new ast_node(node)) {

}

ast::ast(ast_node_qword node) : type(NodeType::QWORD), node(new ast_node(node)) {

}

ast::ast(ast_node_string node) : type(NodeType::STRING), node(new ast_node(node)) {

}

ast::ast(ast_node_array node) : type(NodeType::ARRAY), node(new ast_node(node)) {

}

ast::ast(ast_node_unary node, bool post)
    : type(post ? NodeType::POST_UNARY : NodeType ::PRE_UNARY), node(new ast_node(node)) {
    
}

ast::ast(ast_node_binary node) : type(NodeType::BINARY), node(new ast_node(node)) {

}

ast::ast(ast_node_block node) : type(NodeType::BLOCK), node(new ast_node(node)) {

}

ast::ast(ast_node_function node) : type(NodeType::FUNCTION), node(new ast_node(node)) {

}

ast::ast(ast_node_struct node) : type(NodeType::STRUCT), node(new ast_node(node)) { }

ast_node_none& ast::get_none() {
    return std::get<0>(*node);
}

ast_node_symbol& ast::get_symbol() {
    return std::get<1>(*node);
}

ast_node_byte& ast::get_byte() {
    return std::get<2>(*node);
}

ast_node_word& ast::get_word() {
    return std::get<3>(*node);
}

ast_node_dword& ast::get_dword() {
    return std::get<4>(*node);
}

ast_node_qword& ast::get_qword() {
    return std::get<5>(*node);
}

ast_node_string& ast::get_string() {
    return std::get<6>(*node);
}

ast_node_array& ast::get_array() {
    return std::get<7>(*node);
}

ast_node_unary& ast::get_unary() {
    return std::get<8>(*node);
}

ast_node_binary& ast::get_binary() {
    return std::get<9>(*node);
}

ast_node_block& ast::get_block() {
    return std::get<10>(*node);
}

ast_node_function& ast::get_function() {
    return std::get<11>(*node);
}

ast_node_struct& ast::get_struct() {
    return std::get<12>(*node);
}

ptype ast::get_type() {
    switch(type) {
        case NodeType::NONE: [[fallthrough]];
        case NodeType::BLOCK: return {0};
        case NodeType::SYMBOL: return {get_symbol().symbol->get_type()};
        case NodeType::BYTE: return get_byte().type;
        case NodeType::WORD: return get_word().type;
        case NodeType::DWORD: return get_dword().type;
        case NodeType::QWORD: return get_qword().type;
        case NodeType::STRING: return {TypeID::STRING, 0};
        case NodeType::ARRAY: return get_array().type;
        case NodeType::STRUCT: return get_struct().type;
        case NodeType::PRE_UNARY: [[fallthrough]];
        case NodeType::POST_UNARY: return get_unary().type;
        case NodeType::BINARY: return get_binary().type;
        case NodeType::FUNCTION: return get_function().type;
    }
    return {0};
}