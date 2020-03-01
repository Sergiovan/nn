#include "common/ast.h"

#include <cstring>

void delete_list(list<ast> l) {
    ast* next{l.head};
    while (next) {
        ast* del = next;
        next = next->next;
        delete del;
    }
    l.count = 0;
}

list<ast> clone_list(list<ast> l) {
    list<ast> ret;
    ret.head = ast::make(l.head->clone());
    ast* n = ret.head, *c = l.head;
    while (c->next) {
        n->next = ast::make(c->next->clone());
        n->next->prev = n;
        n = n->next;
        c = c->next;
    }
    ret.tail = n;
    ret.count = l.count;
    return ret;
}

ast_none ast_none::clone() const {
    return {};
}

ast_zero ast_zero::clone() const {
    return {sym};
}

ast_unary::~ast_unary() {
    ASSERT(node, "Unary node was null");
    delete node;
}

ast_unary ast_unary::clone() const {
    return {sym, ast::make(node->clone()), post};
}

ast_binary::~ast_binary() {
    ASSERT(left, "Binary left node was null");
    ASSERT(right, "Binary right node was null");
    delete left;
    delete right;
}

ast_binary ast_binary::clone() const {
    return {sym, ast::make(left->clone()), ast::make(right->clone())};
}

ast_value ast_value::clone() const {
    return {value};
}

ast_string::~ast_string() {
    ASSERT(chars, "String chars was null");
    delete [] chars;
}

ast_string ast_string::clone() const {
    ast_string ret{new u8[length], length};
    std::memcpy(ret.chars, chars, length);
    return ret;
}

ast_compound::~ast_compound() {
    delete_list(elems);
}

ast_compound ast_compound::clone() const {
    ast_compound ret{};
    ret.elems = clone_list(elems);
    return ret;
}

ast_nntype ast_nntype::clone() const {
    return {t}; // TBD
}

ast_block::~ast_block() {
    delete_list(elems);
    delete_list(at_end);
}

ast_block ast_block::clone() const {
    ast_block ret{};
    ret.elems = clone_list(elems);
    ret.at_end = clone_list(at_end);
    return ret;
}

ast_iden ast_iden::clone() const {
    return {s}; // TBD
}

ast::ast() : tt{ast_type::NONE}, none{} {
    
}

ast::ast(ast_type tt, token* tok, type* t) : tt{tt}, tok{tok}, t{t} {
    
}

ast::~ast() {
    switch (tt) {
        case ast_type::UNARY:
            unary.~ast_unary();
            break;
        case ast_type::BINARY:
            binary.~ast_binary();
            break;
        case ast_type::STRING:
            string.~ast_string();
            break;
        case ast_type::BLOCK: [[fallthrough]];
        case ast_type::COMPOUND:
            compound.~ast_compound();
            break;
        default:
            break;
    }
}

ast ast::clone() const {
    ast ret{};
    ret.tt = tt;
    switch (tt) {
        case ast_type::NONE:
            break;
        case ast_type::ZERO:
            ret.zero = zero.clone();
            break;
        case ast_type::UNARY:
            ret.unary = unary.clone();
            break;
        case ast_type::BINARY:
            ret.binary = binary.clone();
            break;
        case ast_type::VALUE:
            ret.value = value.clone();
            break;
        case ast_type::STRING:
            ret.string = string.clone();
            break;
        case ast_type::COMPOUND:
            ret.compound = compound.clone();
            break;
        case ast_type::TYPE:
            ret.nntype = nntype.clone();
            break;
        case ast_type::BLOCK:
            ret.block = block.clone();
            break;
        case ast_type::IDENTIFIER:
            ret.iden = iden.clone();
            break;
    }
    
    return ret;
}

bool ast::is_none() {
    return tt == ast_type::NONE;
}

bool ast::is_zero() {
    return tt == ast_type::ZERO;
}

bool ast::is_unary() {
    return tt == ast_type::UNARY;
}

bool ast::is_binary() {
    return tt == ast_type::BINARY;
}

bool ast::is_value() {
    return tt == ast_type::VALUE;
}

bool ast::is_string() {
    return tt == ast_type::STRING;
}

bool ast::is_compound() {
    return tt == ast_type::COMPOUND;
}

bool ast::is_nntype() {
    return tt == ast_type::TYPE;
}

bool ast::is_block() {
    return tt == ast_type::BLOCK;
}

bool ast::is_iden() {
    return tt == ast_type::IDENTIFIER;
}

ast* ast::make(const ast& o) {
    return new ast{o.clone()};
}

ast* ast::make_none(const ast_none& n, token* tok, type* t) {
    ast* ret = new ast{ast_type::NONE, tok, t};
    ret->none = n;
    ret->compiletime = true;
    return ret;
}

ast* ast::make_zero(const ast_zero& z, token* tok, type* t) {
    ast* ret = new ast{ast_type::ZERO, tok, t};
    ret->zero = z;
    return ret;
}

ast* ast::make_unary(const ast_unary& u, token* tok, type* t) {
    ast* ret = new ast{ast_type::UNARY, tok, t};
    ret->unary = u;
    return ret;
}

ast* ast::make_binary(const ast_binary& b, token* tok, type* t) {
    ast* ret = new ast{ast_type::BINARY, tok, t};
    ret->binary = b;
    return ret;
}

ast* ast::make_value(const ast_value& v, token* tok, type* t) {
    ast* ret = new ast{ast_type::VALUE, tok, t};
    ret->value = v;
    ret->compiletime = true;
    return ret;
}

ast* ast::make_string(const ast_string& s, token* tok, type* t) {
    ast* ret = new ast{ast_type::STRING, tok, t};
    ret->string = s;
    ret->compiletime = true;
    return ret;
}

ast* ast::make_compound(const ast_compound& c, token* tok, type* t) {
    ast* ret = new ast{ast_type::COMPOUND, tok, t};
    ret->compound = c;
    return ret;
}

ast* ast::make_nntype(const ast_nntype& t, token* tok, type* typ) {
    ast* ret = new ast{ast_type::TYPE, tok, typ};
    ret->nntype = t;
    ret->compiletime = true;
    return ret;
}

ast* ast::make_block(const ast_block& c, token* tok, type* t) {
    ast* ret = new ast{ast_type::BLOCK, tok, t};
    ret->block = c;
    return ret;
}

ast* ast::make_iden(const ast_iden& i, token* tok, type* t) {
    ast* ret = new ast{ast_type::IDENTIFIER, tok, t};
    ret->iden = i;
    return ret;
}

