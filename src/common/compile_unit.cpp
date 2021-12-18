#include "compile_unit.h"

#include "ast.h"
#include "symbol_table.h"
#include "util.h"

compile_unit::compile_unit(ast* base, symbol_table* st, symbol* context) 
    : base{base}, st{st}, levels{{{base, st}}}, current{base}, context{context} {

}

void compile_unit::push(ast* base, symbol_table* st) {
    levels.emplace(base, st);
}

void compile_unit::pop() {
    levels.pop();
}

compile_unit::ctx& compile_unit::top() {
    return levels.top();
}

bool compile_unit::circular_dependency() {
    if (status == STALLED && dependency) {
        std::set<compile_unit*> found{this};
        
        return dependency->circular_dependency(found);
        
    } else {
        return false;
    }
}

compile_unit::compile_unit_status compile_unit::update_children() {
    if (children.empty()) {
        return DONE;
    } else {
        compile_unit_status res = DONE;
        for (auto child : children) {
            switch (child->status) {
                case READY:
                    if (res == DONE) {
                        res = READY;
                    }
                    break;
                case DONE:
                    break;
                case STALLED:
                    res = STALLED;
                    break;
                case BLOCKED:
                    return BLOCKED;
            }
        }
        return res;
    }
}

bool compile_unit::circular_dependency(std::set<compile_unit*> found) {
    if (found.contains(this) || status == BLOCKED) {
        return true;
    } else if (!dependency || status != STALLED) {
        return false;
    } else {
        found.insert(this);
        return dependency->circular_dependency(found);
    }
}
