#pragma once

#include "common/ast_walker.h"
#include "common/convenience.h"
#include "common/ir.h"
#include "common/grammar.h"

#include <map>
#include <vector>

struct type;
struct st_entry;
struct ast_unary;
struct ast_binary;
struct overload;
class type_table;
class symbol_table;

namespace ast_compiler_ns {
    
struct declaration {
    u64 offset;
    u64 size;
    type* t;
    st_entry* entry;
    ir_triple* pos;
};

struct context {
    type* function_type{nullptr};
    ir_triple* function_start{nullptr};
    ir_triple* function_end{nullptr};
    ir_triple* function_returns{nullptr};
    
    ir_triple* block_start{nullptr};
    ir_triple* block_end{nullptr};
    
    ir_triple* loop_start{nullptr};
    ir_triple* loop_end{nullptr};
    ir_triple* loop_breaks{nullptr};
    ir_triple* loop_continues{nullptr};
    
    ir_triple* try_value{nullptr};
    ir_triple* try_catch{nullptr};
    
    bool in_try{false};
    bool in_switch{false};
    
    bool breaks{false};
    bool continues{false};
    bool returns{false};
    
    std::vector<declaration> declarations{};
    
    context down(); // Properties that flow downwards
    void clear(); // Clear upwards properties when moving sideways
    void update(context& o); // Properties that flow upwards
}; 


}

class ast_compiler {
public:
    ast_compiler(ast* root, symbol_table* st, type_table& tt);
    ~ast_compiler();
    
    void compile();
    ir_triple* get();
private:
    ir_triple* build(symbol_table* st, ast_compiler_ns::context& ctx);
    ir_triple* build(ast_unary& un, symbol_table* st, ast_compiler_ns::context& ctx);
    ir_triple* build(ast_binary& bin, symbol_table* st, ast_compiler_ns::context& ctx);
    
    ir_triple* make();
    ir_triple* make(const ir_triple& triple, type* t = nullptr);
    
    ir_triple* next();
    ir_triple* next(ir_triple* triple);
    ir_triple* next(const ir_triple& triple, type* t = nullptr);
    
    ir_triple* jump();
    ir_triple* jump_to(ir_triple* other);
    
    ir_triple* ahead();
    ir_triple* ahead(const ir_triple& triple, type* t = nullptr);
    
    void move_to(ir_triple* before_begin, ir_triple* end, ir_triple* after_this);
    
    void fix_jump(ir_triple* cond);
    void fix_declarations(const std::vector<ast_compiler_ns::declaration>& decls);
    
    ir_op::code grammar_symbol_to_ir_op(Grammar::Symbol sym);
    ir_op::code conversion_operator(type* from, type* to);
    
    ast_walker walker;
    
    ir_triple* root;
    symbol_table* st;
    type_table& tt;
    
    ir_triple* c{nullptr};
    
    std::vector<ir_triple*> triples{};
    std::map<st_entry*, ast_compiler_ns::declaration> offset_info{};
    std::map<ast*, ir_triple*> functions{};
    std::map<ast*, ir_triple*> labels{};
};
