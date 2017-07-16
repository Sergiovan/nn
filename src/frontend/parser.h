#pragma once

#include "tokenizer.h"
#include "common/ast.h"
#include "common/globals.h"
#include "common/grammar.h"

class parser_exception : std::exception {
    // TODO Stuff, of course
};

/* For things that should not be possible */
class parser_error : std::exception {

};

/* CONVENTION: Each parsing function expects to start on its first token, and 
   should end after its last token, unless exceptions occur */
class parser {
public:
    parser(globals& g);
    ast* parse(tokenizer& t);
    
    token next() noexcept;
    bool test(Grammar::TokenType type, int lookahead = 0) noexcept; // Just 0 and 1 for now
    bool test(Grammar::Keyword keyword, int lookahead = 0) noexcept; // Just 0 and 1 for now
    bool test(Grammar::Symbol symbol, int lookahead = 0) noexcept; // Just 0 and 1 for now
    
    bool is(Grammar::TokenType type);
    bool is(Grammar::Keyword keyword);
    bool is(Grammar::Symbol symbol);
    
    bool peek(Grammar::TokenType type);
    bool peek(Grammar::Keyword keyword);
    bool peek(Grammar::Symbol symbol);
    
    void require(Grammar::TokenType type);
    void require(Grammar::Keyword keyword);
    void require(Grammar::Symbol symbol);
    
    inline symbol_table& st() { return g.get_symbol_table(); };
    inline type_table& tt() { return g.get_type_table(); };
    
    ast* iden();
    ast* compileriden();
    
    ast* number();
    ast* string();
    ast* character();
    ast* array();
    ast* struct_lit();
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
    
    ast* vardeclperiod();
    ast* vardecl();
    
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
    ast* assignment(ast* assignee = nullptr);
    
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
    
    bool boolean(uid type);
    
    tokenizer* t;
    globals& g;
    token c, n;
    uid context = 0; // What we are expecting
    
    struct symbol_guard {
        symbol_guard(globals& g);
        ~symbol_guard();
        
        symbol_table* nst;
        symbol_table* ost;
        
        globals& g;
    };
    
    struct context_guard {
        context_guard(uid& context);
        ~context_guard();
        
        uid& context;
        uid pcontext;
    };
};
