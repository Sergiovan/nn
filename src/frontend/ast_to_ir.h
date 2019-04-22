#pragma once

#include <stack>
#include "common/ir.h"
#include "frontend/parser.h"


struct ast;
class symbol_table;

class ir_builder {
public:
    ir_builder(parse_info& p);
    void build(ast* ast = nullptr, symbol_table* sym = nullptr);
    void optimize();
private:    
    void add_proper(ir_triple& triple, ast* ast, symbol_table* sym);
    ir* new_ir();
    
    parse_info& p;
    ir* base{nullptr};
    ir* current{nullptr};
    std::vector<ir*> irs{};
    std::stack<block> blocks{};
};
