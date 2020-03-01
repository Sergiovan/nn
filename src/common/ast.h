#pragma once

#include "common/grammar.h"
#include "common/list.h"
#include "common/token.h"

enum class ast_type : u8 {
    NONE, ZERO, UNARY, BINARY,
    VALUE, STRING, COMPOUND, TYPE, 
    BLOCK, IDENTIFIER
};

struct ast;
struct type;
struct symbol;

struct ast_none { 
    ast_none clone() const;
}; // LITERALLY FUCKALL

struct ast_zero {
    grammar::symbol sym;
    
    ast_zero clone() const;
};

struct ast_unary {
    grammar::symbol sym;
    ast* node;
    bool post{false};
    
    ~ast_unary();
    ast_unary clone() const;
};

struct ast_binary {
    grammar::symbol sym;
    ast* left;
    ast* right;
    
    ~ast_binary();
    ast_binary clone() const;
};

struct ast_value {
    u64 value;
    
    ast_value clone() const;
};


struct ast_string {
    u8* chars;
    u64 length;
    
    ~ast_string();
    ast_string clone() const;
};

struct ast_compound {
    list<ast> elems;
    ~ast_compound();
    ast_compound clone() const;
};

struct ast_nntype {
    // Special, tbd
    type* t;
    
    ast_nntype clone() const;
};

struct ast_block {
    list<ast> elems;
    list<ast> at_end;
    ~ast_block();
    ast_block clone() const;
};

struct ast_iden {
    // Special, tbd
    symbol* s;
    
    ast_iden clone() const;
};

struct ast : public list_node<ast> {
    ast_type tt;    
    token* tok;
    type* t;
    
    union {
        ast_none none;
        ast_zero zero;
        ast_unary unary;
        ast_binary binary;
        ast_value value;
        ast_string string;
        ast_compound compound;
        ast_nntype nntype;
        ast_block block;
        ast_iden iden;
    };
    
    bool compiletime{false}; // The value of this is known at compiletime
    
    ast();
    ast(ast_type tt, token* tok, type* t);
    ~ast();
    
    ast clone() const;
    
    bool is_none();
    bool is_zero();
    bool is_unary();
    bool is_binary();
    bool is_value();
    bool is_string();
    bool is_compound();
    bool is_nntype();
    bool is_block();
    bool is_iden();
    
    static ast* make(const ast& o);
    static ast* make_none(const ast_none& n, token* tok, type* t);
    static ast* make_zero(const ast_zero& z, token* tok, type* t);
    static ast* make_unary(const ast_unary& u, token* tok, type* t);
    static ast* make_binary(const ast_binary& b, token* tok, type* t);
    static ast* make_value(const ast_value& v, token* tok, type* t);
    static ast* make_string(const ast_string& s, token* tok, type* t);
    static ast* make_compound(const ast_compound& c, token* tok, type* t);
    static ast* make_nntype(const ast_nntype& t, token* tok, type* typ);
    static ast* make_block(const ast_block& c, token* tok, type* t);
    static ast* make_iden(const ast_iden& i, token* tok, type* t);
};
