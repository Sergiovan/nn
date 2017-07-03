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
    
    token next() noexcept ;
    bool test(Grammar::TokenType type, int lookahead = 0) noexcept ; // Just 0 and 1 for now
    void require(Grammar::TokenType type);
    
    ast* iden();
    ast* compileriden();
    
    ast* string();
    ast* character();
    ast* array();
    ast* strict_lit();
    ast* literal();
    
    ast* program();
    ast* statements();
    ast* statement();
    
    ast* compileropts();
    ast* scope();
    
    ast* ifstmt();
    ast* ifsemi();
    ast* ifstmt2();
    ast* elsestmt();
    
private:
    tokenizer* t;
    globals& g;
    token c, n;
};
