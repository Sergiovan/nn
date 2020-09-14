#include "common/ast.h"

#include <cstring>
#include <sstream>
#include <iomanip>
#include "common/util.h"
#include "common/type.h"

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
    ret.head = l.head->clone();
    ast* n = ret.head, *c = l.head;
    while (c->next) {
        n->next = c->next->clone();
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

void ast_unary::clean() {
    ASSERT(node, "Unary node was null");
    delete node;
}

ast_unary ast_unary::clone() const {
    return {sym, node->clone(), post};
}

void ast_binary::clean() {
    ASSERT(left, "Binary left node was null");
    ASSERT(right, "Binary right node was null");
    delete left;
    delete right;
}

ast_binary ast_binary::clone() const {
    return {sym, left->clone(), right->clone()};
}

ast_value ast_value::clone() const {
    return {value};
}

ast_string::ast_string(const char* c) {
    length = std::strlen(c);
    chars = new u8[length];
    std::memcpy(chars, c, length);
}

ast_string::ast_string(const std::string& s) {
    length = s.length();
    chars = new u8[length];
    std::memcpy(chars, s.data(), length);
}

ast_string::ast_string(u8* chars, u64 length) : chars{chars}, length{length} {
    
}

void ast_string::clean() {
    ASSERT(chars, "String chars was null");
    delete [] chars;
}


ast_string ast_string::clone() const {
    ast_string ret{new u8[length], length};
    std::memcpy(ret.chars, chars, length);
    return ret;
}

void ast_compound::clean() {
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

void ast_block::clean() {
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
            unary.clean();
            unary.~ast_unary();
            break;
        case ast_type::BINARY:
            binary.clean();
            binary.~ast_binary();
            break;
        case ast_type::STRING:
            string.clean();
            string.~ast_string();
            break;
        case ast_type::BLOCK: 
            block.clean();
            block.~ast_block();
            break;
        case ast_type::COMPOUND:
            compound.clean();
            compound.~ast_compound();
            break;
        default:
            break;
    }
    
    if (compiled && compiled != this) {
        delete compiled;
    }
}

ast* ast::clone() const {
    ast* ret = new ast{};
    ret->tt = tt;
    ret->tok = tok;
    ret->t = t;
    
    switch (tt) {
        case ast_type::NONE:
            break;
        case ast_type::ZERO:
            ret->zero = zero.clone();
            break;
        case ast_type::UNARY:
            ret->unary = unary.clone();
            break;
        case ast_type::BINARY:
            ret->binary = binary.clone();
            break;
        case ast_type::VALUE:
            ret->value = value.clone();
            break;
        case ast_type::STRING:
            ret->string = string.clone();
            break;
        case ast_type::COMPOUND:
            ret->compound = compound.clone();
            break;
        case ast_type::TYPE:
            ret->nntype = nntype.clone();
            break;
        case ast_type::BLOCK:
            ret->block = block.clone();
            break;
        case ast_type::IDENTIFIER:
            ret->iden = iden.clone();
            break;
    }
    
    ret->precedence = precedence;
    ret->inhprecedence = inhprecedence;
    ret->compiletime = compiletime;
    
    if (compiled) {
        if (compiled == this) {
            ret->compiled = ret;
        } else {
            ret->compiled = compiled->clone();
        }
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
    return o.clone();
}

ast* ast::make_none(const ast_none& n, token* tok, type* t) {
    ast* ret = new ast{ast_type::NONE, tok, t};
    ret->none = n;
    ret->compiletime = true;
    ret->compiled = ret;
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
    ret->compiled = ret;
    return ret;
}

ast* ast::make_string(const ast_string& s, token* tok, type* t) {
    ast* ret = new ast{ast_type::STRING, tok, t};
    ret->string = s;
    ret->compiletime = true;
    ret->compiled = ret;
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
    ret->compiled = ret;
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
    ret->compiled = ret;
    return ret;
}

std::string ast::to_simple_string() {
    return ss::get() << tt << " [" << t->to_string() 
                     << "] " << " '" << tok->content.substr(0, 40) 
                     << "' " << ss::end();
}

std::string ast::to_string(bool recursive, u64 level) {
    std::stringstream ss{};
    std::string pad(level, '*');
    ss << pad;
    
    ss << " " << to_simple_string();
    
    switch (tt) {
        case ast_type::NONE:
            ss << "()";
            break;
        case ast_type::ZERO:
            ss << "(" << zero.sym << ")";
            break;
        case ast_type::UNARY:
            ss << "(";
            if (unary.post) {
                if (recursive) {
                    ss << "\n" << unary.node->to_string(recursive, level + 1) << "\n" << pad << "> " << unary.sym 
                       << "\n" << pad << " )";
                } else {
                    ss << unary.node->to_simple_string() << unary.sym << ")";
                }
            } else {
                if (recursive) {
                    ss << "\n" << pad << "> " << unary.sym << "\n" << unary.node->to_string(recursive, level + 1) 
                    << "\n" << pad << " )";
                } else {
                    ss << unary.sym << unary.node->to_simple_string() << ")";
                }
            }
            break;
        case ast_type::BINARY:
            ASSERT(binary.left, "Left node was nullptr");
            ASSERT(binary.right, "Right node was nullptr");
            ss << "(";
            if (recursive) {
                ss << "\n" << binary.left->to_string(recursive, level + 1) << "\n" << pad << "> " << binary.sym 
                   << "\n" << binary.right->to_string(recursive, level + 1) << "\n" << pad << " )";
            } else {
                ss << binary.left->to_simple_string() << binary.sym << binary.right->to_simple_string() << ")";
            }
            break;
        case ast_type::VALUE:
            ss << "(" << value.value << ")";
            break;
        case ast_type::STRING:
            ss << "(" << std::string(reinterpret_cast<char*>(string.chars), string.length) << ")";
            break;
        case ast_type::COMPOUND:
            ss << "(";
            if (recursive) {
                for (auto e : compound.elems) {
                    ss << "\n" << e->to_string(recursive, level + 1);
                }
            } else {
                for (auto e : compound.elems) {
                    ss << "\n" << pad << e->to_simple_string();
                }
            }
            ss << "\n" << pad << " )";
            break;
        case ast_type::BLOCK:
            ss << "(";
            if (recursive) {
                for (auto e : block.elems) {
                    ss << "\n" << e->to_string(recursive, level + 1);
                }
                ss << "\n" << pad << " ---";
                for (auto e : block.at_end) {
                    ss << "\n" << pad << e->to_string(recursive, level + 1);
                }
            } else {
                for (auto e : block.elems) {
                    ss << "\n" << e->to_simple_string();
                }
                ss << "\n" << pad << " ---";
                for (auto e : block.at_end) {
                    ss << "\n" << pad << e->to_simple_string();
                }
            }
            ss << "\n" << pad << " )";
            break;
        case ast_type::TYPE:
            ss << "(" << nntype.t->to_string() << ")";
            break;
        case ast_type::IDENTIFIER:
            ss << "(IDENTIFIER)"; // TODO
            break;
    }
    
    return ss.str();
}

token* ast::get_leftmost_token() {
    switch (tt) {
        case ast_type::NONE: [[fallthrough]];
        case ast_type::ZERO: [[fallthrough]];
        case ast_type::VALUE: [[fallthrough]];
        case ast_type::STRING: [[fallthrough]];
        case ast_type::TYPE: [[fallthrough]];
        case ast_type::IDENTIFIER:
            return tok;
        case ast_type::UNARY:
            return token::leftmost(tok, unary.node->get_leftmost_token());
        case ast_type::BINARY: { 
            token* ll = binary.left->get_leftmost_token();
            token* rl = binary.right->get_leftmost_token();
            return token::leftmost(tok, token::leftmost(ll, rl));
        }
        case ast_type::COMPOUND: {
            token* leftmost = tok;
            for (auto elem : compound.elems) {
                leftmost = token::leftmost(leftmost, elem->get_leftmost_token());
            }
            return leftmost;
        }
        case ast_type::BLOCK: [[fallthrough]];
        default:
            return tok;
    }
}

token* ast::get_rightmost_token() {
    switch (tt) {
        case ast_type::NONE: [[fallthrough]];
        case ast_type::ZERO: [[fallthrough]];
        case ast_type::VALUE: [[fallthrough]];
        case ast_type::STRING: [[fallthrough]];
        case ast_type::TYPE: [[fallthrough]];
        case ast_type::IDENTIFIER:
            return tok;
        case ast_type::UNARY:
            return token::rightmost(tok, unary.node->get_rightmost_token());
        case ast_type::BINARY:{ 
            token* lr = binary.left->get_rightmost_token();
            token* rr = binary.right->get_rightmost_token();
            return token::rightmost(tok, token::rightmost(lr, rr));
        }
        case ast_type::COMPOUND: {
            token* rightmost = tok;
            for (auto elem : compound.elems) {
                rightmost = token::rightmost(rightmost, elem->get_rightmost_token());
            }
            return rightmost;
        }
        case ast_type::BLOCK: [[fallthrough]];
        default:
            return tok;
    }
}

std::ostream& operator<<(std::ostream& os, ast_type t) {
    switch (t) {
        case ast_type::NONE:
            return os << "NONE";
        case ast_type::ZERO:
            return os << "ZERO";
        case ast_type::UNARY:
            return os << "UNARY";
        case ast_type::BINARY:
            return os << "BINARY";
        case ast_type::VALUE:
            return os << "VALUE";
        case ast_type::STRING:
            return os << "STRING";
        case ast_type::COMPOUND:
            return os << "COMPOUND";
        case ast_type::TYPE:
            return os << "TYPE";
        case ast_type::BLOCK:
            return os << "BLOCK";
        case ast_type::IDENTIFIER:
            return os << "IDENTIFIER";
        default:
            return os << "INVALID AST";
    }
}
