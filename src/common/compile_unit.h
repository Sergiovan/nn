#pragma once

#include <set>
#include <stack>
#include <vector>

struct ast;
struct symbol;
class symbol_table;

struct compile_unit {
    enum compile_unit_status {
        READY, // Ready to go
        STALLED, // Waiting for something else to resolve
        BLOCKED, // Can never resolve, circular dependency
        DONE // Finished properly
    };
    
    struct ctx {
        ast* base;
        symbol_table* st;
    };
    
    compile_unit(ast* base, symbol_table* st, symbol* context);
    
    void push(ast* base, symbol_table* st);
    void pop();
    
    ctx& top();
    
    bool circular_dependency();
    compile_unit_status update_children();
    
    ast* base; // Original ast
    symbol_table* st; // Original st
    
    std::stack<ctx> levels; // Block nesting
    ast* current; // Last worked on ast
    
    symbol* context; 
    std::vector<compile_unit*> children{};
    compile_unit* dependency{nullptr};
    compile_unit_status status{READY};
    
private:
    bool circular_dependency(std::set<compile_unit*> found);
};
