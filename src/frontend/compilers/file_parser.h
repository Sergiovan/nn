#pragma once

#include "frontend/compiler.h"
#include "common/ast.h"

#include <unordered_set>

class file_parser {
public:
    file_parser(compiler& p, nnmodule& mod);
    
    bool read();
    bool pass();
private:    
    token* next(); // pops c, peek(), c = n
    token* next(grammar::symbol expected); // Same but expecting something
    token* peek(); // Sets n to the next proper token
    token* peek(grammar::symbol expected); // returns n if n != c to avoid going too far in
    void clear();
    
    bool is(token_type tt);
    bool is_keyword(grammar::symbol kw);
    bool is_symbol(grammar::symbol sym);
    
    bool require(token_type tt);
    bool require_keyword(grammar::symbol kw);
    bool require_symbol(grammar::symbol sym);
    
    bool is_assign();
    bool require_assign();
    
    ast* number();
    ast* identifier();
    ast* char_lit();
    ast* string_lit();
    
    ast* note();
    ast* array_lit();
    ast* struct_lit();
    ast* tuple_lit();
    
    ast* program_unit();
    ast* freestmt();
    ast* stmt();
    ast* scopestmt();
    
    ast* scope();
    
    ast* optscope();
    
    ast* optvardecls();
    
    ast* ifstmt();
    // ast* ifscope();
    
    ast* forstmt();
    ast* whilestmt();
    ast* whilecond();
    ast* whiledostmt();
    ast* dowhilestmt();
    
    ast* switchstmt();
    ast* switchscope();
    ast* casedecl();
    
    ast* trystmt();
    
    ast* returnstmt();
    ast* raisestmt();
    
    ast* gotostmt();
    ast* labelstmt();
    
    ast* deferstmt();
    ast* breakstmt();
    ast* continuestmt();
    
    ast* importstmt();
    ast* usingstmt();
    
    ast* namespacestmt();
    ast* namespacescope();
    
//     ast* declarator();
    ast* _type();
    ast* inferrable_type(bool* is_infer = nullptr);
    
//     ast* paramtype();
//     ast* rettype();
    
    ast* declstmt();
    ast* defstmt();
    
    ast* maybe_identifier();
    
    ast* vardecl(bool special = false);
    ast* simplevardecl(bool special = false);
    
    ast* functype();
    ast* funclit_or_type();
    ast* funclit();
    ast* funcdef();
    ast* capture_group();
    
    ast* functypesig();
    
    ast* funcparam();
    ast* funcret(bool let);
    
    ast* structtypelitdef();
    ast* structscope();
    ast* structvardecl();
    
    ast* uniontypelitdef();
    ast* unionscope();
    
    ast* enumtypelitdef();
    ast* enumscope();
    
    ast* tupletypelit();
    ast* tupletypelitdef();
    ast* tupletypes();
    
//     ast* typelit();
    ast* typelit_nofunc();
    
    ast* assignment();
    ast* assstmt();
    
    ast* deletestmt();
    
    ast* expressionstmt();
    ast* assorexpr(bool stmt = true);
    
    ast* expression();
    ast* ternaryexpr();
    ast* newexpr();
    
    ast* prefixexpr();
    ast* postfixexpr();
    ast* infixexpr();
    ast* dotexpr();
    ast* postcircumfixexpr();
    ast* function_call();
    ast* access();
    ast* reorder();
    
//     ast* literal();
    ast* literalexpr();
    ast* identifierexpr();
    ast* parenexpr();
    
    ast* select();
    ast* compound_identifier_simple();
    
    ast* make_error_ast(token* t);
    
    compiler& comp;
    nnmodule& mod;
    type_table& tt;

    token* c{nullptr};
    token* n{nullptr};
    list<ast> errors{};
    
    std::unordered_set<grammar::symbol> pre_ops{};
    std::unordered_set<grammar::symbol> post_ops{};
    std::unordered_set<grammar::symbol> infix_ops{};
};
