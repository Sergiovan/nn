#pragma once

#include "tokenizer.h"
#include "common/ast.h"
#include "common/globals.h"
#include "common/grammar.h"

class parser_exception : std::exception {
    // TODO Stuff, of course
};

class parser {
public:
    parser(globals& g);
    ast* parse(tokenizer& t);
    
    token next() noexcept;
    bool test(Grammar::TokenType type, int lookahead = 0) noexcept; // Just 0 and 1 for now
    bool is(Grammar::TokenType type);
    bool peek(Grammar::TokenType type);
    void require(Grammar::TokenType type);
    
    ast* iden();
    ast* compileriden();
    
    ast* string();
    ast* character();
    ast* array();
    ast* strict_lit();
    ast* literal();
    
    ast* program();
    ast* statement();
    ast* scopestatement();
    
    ast* compileropts();
    ast* scope();
    
    ast* ifstmt();
    ast* ifscope();
    
    ast* forstmt();
    ast* forcond();
    
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
    
    ast* namespacestmt();
    
    ast* varclassstmt();
    ast* typeestmt();
    ast* functypestmt();
    
    ast* declstmt();
    
    ast* declstructstmt();
    
    ast* varstmt();
    
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
    tokenizer* t;
    globals& g;
    token c, n;
};
