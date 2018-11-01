#pragma once

#include <stack>
#include <utility>

#include <exception>
#include "frontend/token.h"
#include "common/grammar.h"
#include "common/type_table.h"

struct ast;
class reader;
class lexer;
class symbol_table;
class parser;

enum class epanic_mode {
    SEMICOLON, COMMA, ESCAPE_BRACE, ESCAPE_BRACKET, ESCAPE_PAREN
};

struct context {
    symbol_table* st;
    type* function;
    type* _struct;
    type* aux;
    type* expected;
};

struct ctx_guard {
    ctx_guard(parser& p);
    ~ctx_guard();
    
    void deactivate();
    
    parser& p;
    bool active{true};
};

class parser_exception : public std::exception {
};


class parser{
public:
    ast* parse(lexer* l);
    ast* parse(const std::string& str, bool is_file = false);
    
    symbol_table* get_as_module();
    parser fork();
    
    void print_errors();
private:
    context& ctx();
    symbol_table* st();
    
    context& push_context(bool clear = false);
    context pop_context();
    ctx_guard guard();
    
    ast* error(const std::string& msg, token* t = nullptr, bool panic = true, epanic_mode mode = epanic_mode::SEMICOLON);
    void panic(epanic_mode mode);
    
    token next();
    
    bool is(Grammar::TokenType tt);
    bool is(Grammar::Symbol sym);
    bool is(Grammar::Keyword kw);
    
    bool peek(Grammar::TokenType tt, u64 lookahead = 0);
    bool peek(Grammar::Symbol sym, u64 lookeahead = 0);
    bool peek(Grammar::Keyword kw, u64 lookahead = 0);
    
    bool require(Grammar::TokenType tt, const std::string& err = {});
    bool require(Grammar::Symbol sym, const std::string& err = {});
    bool require(Grammar::Keyword kw, const std::string& err = {});
    
    ast* iden();
    ast* compileriden();
    ast* compileropts();
    ast* compilernote();
    
    ast* number();
    ast* string();
    ast* character();
    ast* array();
    ast* struct_lit();
    ast* literal();
    
    ast* program();
    ast* statement();
    ast* scopestatement();
    ast* scope();
    
    ast* ifstmt();
    ast* ifscope();
        
    ast* forstmt();
    ast* forcond();
    
    ast* whilestmt();
    
    ast* switchstmt();
    ast* switchscope();
    ast* casestmt();
    
    ast* trystmt();
    
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
    ast* namespacescope();
    
    ast* typemod();
    ast* nntype();
    ast* infer();
    
    ast* freedeclstmt();
    ast* structdeclstmt();
    
    ast* vardeclass();
    
    ast* freevardecliden();
    ast* freevardecl();
    
    ast* structvardecliden();
    ast* structvardecl();
    
    ast* funcdecl();
    ast* funcval();
    ast* nnparameter();
    
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
    
    ast* newexpr();
    ast* deleteexpr();
    
    ast* e17();
    ast* e16();
    ast* e15();
    ast* e14();
    ast* e13();
    ast* e12();
    ast* e11();
    ast* e10();
    ast* e9();
    ast* e8();
    ast* e7();
    ast* e6();
    ast* e5();
    ast* e4();
    ast* e3();
    ast* e2();
    ast* e1();
    ast* ee();
    
    ast* argument();
    ast* fexpression();
    ast* mexpression();
    ast* aexpression();
    
    token c;
    lexer* l{nullptr};
    
    std::stack<context> contexts{};
    symbol_table* root_st{};
    type_table types{};
    
    dict<symbol_table*> modules{};
    std::vector<std::pair<token, std::string>> errors{};
    
    std::vector<std::pair<ast*, context>> unfinished{};
    std::vector<ast*> labels{};
    
    friend struct ctx_guard;
};
