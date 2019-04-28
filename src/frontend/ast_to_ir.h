#pragma once

#include <stack>
#include "common/grammar.h"
#include "common/ir.h"
#include "frontend/parser.h"


struct ast;
class symbol_table;

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
    
    ir_triple* current();
    ir_triple* current_end();
    block& current_block();
    
    parse_info& p;
    
    ir_op::code symbol_to_ir_code(Grammar::Symbol sym);
    
    std::vector<ir_triple*> triples{}; // Owned
    std::stack<block> blocks{};
    std::map<st_entry*, ir_triple*> labeled{};
};
