#pragma once

#include <filesystem>
#include <stack>
#include <utility>

#include <exception>
#include "frontend/token.h"
#include "common/grammar.h"
#include "common/type_table.h"
#include "common/symbol_table.h"

struct ast;
class reader;
class lexer;
class symbol_table;
class parser;

enum class epanic_mode {
    NO_PANIC, SEMICOLON, COMMA, 
    ESCAPE_BRACE, ESCAPE_BRACKET, ESCAPE_PAREN,
    IN_ARRAY, IN_STRUCT_LIT,
    ULTRA_PANIC
};

enum class eexpression_type {
    EXPRESSION, ASSIGNMENT, DECLARATION, INVALID
};

struct context {
    symbol_table* st;
    type* function;
    type* _struct;
    type* aux;
    type* expected;
    ast* first_param;
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

struct parse_info {
    ast* result;
    symbol_table* root_st;
    type_table* types;
    dict<symbol_table*> modules;
};

class parser{
public:
    parser();
    parser(type_table& tt);
    ~parser();
    
    parser(const parser&) = delete;
    parser(parser&&) = delete;
    parser& operator=(const parser&) = delete;
    parser& operator=(parser&&) = delete;
    
    parse_info parse(lexer* l);
    parse_info parse(const std::string& str, bool is_file = false);
    
    symbol_table* get_as_module();
    parser fork();
    
    bool has_errors();
    void print_errors();
    void print_info();
    void print_st();
    void print_modules();
    void print_types();
private:
    ast* _parse();
    void finish();
    
    context& ctx();
    symbol_table* st();
    
    context& push_context(bool clear = false);
    context pop_context();
    ctx_guard guard();
    
    ast* error(const std::string& msg, epanic_mode mode = epanic_mode::NO_PANIC, token* t = nullptr);
    ast* operator_error(Grammar::Symbol op, type* t, bool post = true);
    ast* operator_error(Grammar::Symbol op, type* l, type* r);
    void panic(epanic_mode mode);
    
    token next();
    
    bool is(Grammar::TokenType tt);
    bool is(Grammar::Symbol sym);
    bool is(Grammar::Keyword kw);
    
    bool peek(Grammar::TokenType tt, u64 lookahead = 0);
    bool peek(Grammar::Symbol sym, u64 lookeahead = 0);
    bool peek(Grammar::Keyword kw, u64 lookahead = 0);
    
    bool require(Grammar::TokenType tt, epanic_mode mode = epanic_mode::SEMICOLON, const std::string& err = {});
    bool require(Grammar::Symbol sym, epanic_mode mode = epanic_mode::SEMICOLON, const std::string& err = {});
    bool require(Grammar::Keyword kw, epanic_mode mode = epanic_mode::SEMICOLON, const std::string& err = {});
    
    void compiler_assert(Grammar::TokenType tt);
    void compiler_assert(Grammar::Symbol tt);
    void compiler_assert(Grammar::Keyword kw);
    
    token skip(u64 amount = 1);
    bool can_peek_skip_groups(u64 from);
    u64 peek_skip_groups(u64 from = 0);
    
    u64 peek_until(Grammar::TokenType tt, bool skip_groups = true);
    u64 peek_until(Grammar::Symbol sym, bool skip_groups = true);
    u64 peek_until(const std::vector<Grammar::Symbol>& syms, bool skip_groups = true);
    u64 peek_until(Grammar::Keyword kw, bool skip_groups = true);
    
    token skip_until(Grammar::TokenType tt, bool skip_groups = true);
    token skip_until(Grammar::Symbol sym, bool skip_groups = true);
    token skip_until(const std::vector<Grammar::Symbol>& syms, bool skip_groups = true);
    token skip_until(Grammar::Keyword kw, bool skip_groups = true);
    
    ast* iden(bool withthis = false, type** thistype = nullptr);
    ast* compileriden();
    ast* compileropts();
    ast* compilernote();
    
    ast* number();
    ast* string();
    ast* character();
    ast* array();
    ast* struct_lit();
    
    ast* safe_literal();
    ast* compound_literal();
    
    ast* program();
    ast* statement();
    ast* scopestatement();
    ast* scope(etable_owner from = etable_owner::COPY);
    
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
    
    type_flags typemod(bool* any = nullptr);
    ast* arraysize();
    ast* nntype(bool* endswitharray = nullptr);
    ast* functype();
    ast* infer();
    
    ast* freedeclstmt();
    ast* structdeclstmt();
    ast* uniondeclstmt();
    
    ast* vardeclass(); // Requires expected values
    
    ast* freevardecliden(ast* t1 = nullptr);
    ast* freevardecl(ast* t1 = nullptr);
    
    ast* structvardecliden(ast* t1 = nullptr);
    ast* structvardecl(ast* t1 = nullptr);
    
    ast* simplevardecl(ast* t1 = nullptr);
    
    type* funcreturns(ast* t1 = nullptr);
    ast* funcdecl(ast* t1 = nullptr, type* thistype = nullptr);
    ast* funcval();
    parameter nnparameter();
    
    ast* structdecl();
    ast* structscope();
    
    ast* uniondecl();
    ast* unionscope();
    
    ast* enumdecl();
    ast* enumscope();
    
    ast* assstmt();
    ast* assignment();
    
    ast* newinit();
    ast* deletestmt();
    
    ast* expressionstmt();
    ast* expression();
    
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
    
    std::pair<ast*, std::string> argument(bool allow_names, type* ftype = nullptr);
    ast* fexpression(eexpression_type* expression_type = nullptr);
    ast* mexpression(eexpression_type* expression_type = nullptr);
    ast* aexpression(); // Requires expected values
    
    bool is_type();
    bool is_infer();
    bool is_decl_start();
    bool is_compiler_token();
    bool is_assignment_operator(Grammar::Symbol sym = Grammar::Symbol::SYMBOL_INVALID);
    bool is_pointer_type();
    
    type* pointer_to(type* to, eptr_type ptype = eptr_type::NAKED, u64 size = 0);
    
    type* get_result_type(Grammar::Symbol op, type* t);
    type* get_result_type(Grammar::Symbol op, type* lt, type* rt);
    
    token c;
    lexer* l{nullptr};
    
    std::stack<context> contexts{};
    symbol_table* root_st{nullptr};
    type_table& types;
    
    dict<symbol_table*> modules{};
    std::vector<std::pair<token, std::string>> errors{};
    
    std::vector<std::pair<ast*, context>> unfinished{};
    dict<ast*> labels{};
    
    bool forked{false};
    std::filesystem::path file_path{std::filesystem::current_path()};
    
    friend struct ctx_guard;
};

