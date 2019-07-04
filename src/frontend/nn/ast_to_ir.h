#pragma once

#include <stack>
#include "common/grammar.h"
#include "common/ir.h"
#include "frontend/nn/parser.h"


struct ast;
struct type;
class symbol_table;

struct ir_builder_context {
    type* ftype{nullptr};
    u64 return_amount{0};
    bool in_try{false};
    bool safe_to_exit{true};
    bool prev_safe_to_exit{true};
    
    ir_triple* fun_returning{nullptr};
    ir_triple* fun_return{nullptr};
    
    ir_triple* loop_breaking{nullptr};
    ir_triple* loop_continue{nullptr};
};

class ir_builder {
public:
    ir_builder(parse_info& p);
    void build(ast* ast = nullptr, symbol_table* sym = nullptr);
    void optimize();
    ir_triple* get();
private:
    ir_triple* add(ir_triple t);
    ir_triple* add(ir_triple* tp);
    ir_triple* add(ir_op::code code);
    ir_triple* add(ir_op::code code, symbol_table* sym, ast* param1, ast* param2 = nullptr);
    ir_triple* add(ir_op::code code, ir_triple::ir_triple_param param1, ir_triple::ir_triple_param param2 = (ast*) nullptr);
    
    ir_triple* create(ir_op::code code);
    ir_triple* create(ir_op::code code, symbol_table* sym, ast* param1, ast* param2 = nullptr);
    ir_triple* create(ir_op::code code, ir_triple::ir_triple_param param1, ir_triple::ir_triple_param param2 = (ast*) nullptr);
    
    void start_block();
    void end_block();
    
    void start_context();
    void end_context();
    
    ir_triple* current();
    ir_triple* current_end();
    block& current_block();
    
    ir_builder_context& ctx();
    
    parse_info& p;
    
    ir_op::code symbol_to_ir_code(Grammar::Symbol sym);
    ir_op::code conversion_operator(type* from, type* to);
    
    std::vector<ir_triple*> triples{}; // Owned
    std::stack<block> blocks{};
    std::stack<ir_builder_context> contexts{};
    std::map<st_entry*, ir_triple*> labeled{};
};
