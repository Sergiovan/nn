#pragma once

#include <exception>
#include <common/globals.h>
#include "tokenizer.h"

class parser;

class parser_exception : std::exception {
public:
    parser_exception();
    parser_exception(tokenizer* t, std::string message, int cb = -1, int cf = -1, int max_chars = 80);

    virtual const char * what() const noexcept;
    std::string message;
};

/* For things that should not be possible */
class parser_error : std::exception {
public:
    parser_error();
    parser_error(std::string str);

    virtual const char * what() const noexcept;
    std::string message;
};

struct context {
    symbol_table* st;
    ptype expected;
};

struct context_guard {
    context_guard(parser& p);
    ~context_guard();
    parser& p;
};

/* CONVENTION: Each parsing function expects to start on its first token, and
   should end after its last token, unless exceptions occur */
class parser {
public:
    parser(globals& g);
    ast* parse(tokenizer& t);

    token next() noexcept;

    bool is(Grammar::TokenType type) noexcept;
    bool is(Grammar::Keyword keyword) noexcept;
    bool is(Grammar::Symbol symbol) noexcept;

    bool peek(Grammar::TokenType type, u32 lookahead = 0) noexcept;
    bool peek(Grammar::Keyword keyword, u32 lookahead = 0) noexcept;
    bool peek(Grammar::Symbol symbol, u32 lookahead = 0) noexcept;

    void require(Grammar::TokenType type, std::string error = "No message yet");
    void require(Grammar::Keyword keyword, std::string error = "No message yet");
    void require(Grammar::Symbol symbol, std::string error = "No message yet");

    inline symbol_table& st() { return *cx.top().st; }
    inline ptype& etype() {return cx.top().expected; }
    inline type_table& tt() { return g.get_type_table(); }

    context& pop_ctx();
    void push_ctx(ptype type = {}, symbol_table* st = nullptr);

    ast* iden();
    ast* compileriden(); // TODO

    ast* number();
    ast* string();
    ast* character();
    ast* array();
    ast* struct_lit();
    ast* literal();

    ast* program();
    ast* statement();
    ast* scopestatement();

    ast* compileropts(); // TODO
    ast* scope();

    ast* ifstmt();
    ast* ifscope();

    ast* forstmt();
    ast* forcond(); // TODO Refiddle when vardeclperiod() vardeclass() and assignment() have been touched

    ast* whilestmt();

    ast* switchstmt();
    ast* switchscope();
    ast* casestmt();

    ast* returnstmt();

    ast* raisestmt();

    ast* gotostmt();

    ast* labelstmt();

    ast* deferstmt();

    ast* breakstmt();

    ast* continuestmt();

    ast* leavestmt();

    ast* importstmt();

    ast* usingstmt();

    ast* namespacescope();
    ast* namespacestmt();

    ast* varclass();
    ast* type(); // TODO Maybe they don't have to return ast
    ast* propertype();
    ast* functype();

    ast* declstmt();

    ast* declstructstmt();

    ast* vardeclperiod();

    ast* vardecl();
    ast* vardeclass();

    ast* vardeclstruct();

    ast* funcdecl();
    ast* parameter();

    ast* funcval();

    ast* structdecl();
    ast* structscope();

    ast* uniondecl();
    ast* unionscope();

    ast* enumdecl();
    ast* enumscope();

    ast* assstmt();
    ast* assignment();

    ast* newexp();
    ast* deleteexp(); // TODO Fix syntax

    ast* expressionstmt();
    ast* expression();

    ast* e17();
    ast* e17p();

    ast* e16();
    ast* e16p();

    ast* e15();
    ast* e15p();

    ast* e14();
    ast* e14p();

    ast* e13();
    ast* e13p();

    ast* e12();
    ast* e12p();

    ast* e11();
    ast* e11p();

    ast* e10();
    ast* e10p();

    ast* e9();
    ast* e9p();

    ast* e8();
    ast* e8p();

    ast* e7();
    ast* e7p();

    ast* e6();
    ast* e6p();

    ast* e5();
    ast* e5p();

    ast* e4();
    ast* e4p();

    ast* e3();

    ast* e2();
    ast* e2p();

    ast* e1();
    ast* e1p();

    ast* ee();

    ast* argument();
    ast* fexpression();
    ast* mexpression();
    ast* aexpression();
private:
    bool is_type();
    bool is_varclass();
    bool is_literal();
    bool is_expression();
    bool is_fexpression();
    bool boolean(ptype type);


    tokenizer* t{nullptr};
    globals& g;
    std::stack<context> cx;
    token c;
    ptype aux{};

    // Maps "mangled" types to uid
    std::map<std::string, uid> mtt{};
};
