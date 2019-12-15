#pragma once

#include <stack>

struct ast;

class ast_walker {
public:
    ast_walker(ast* root);
    
    ast* next();
private:    
    ast* current;
    
    std::stack<ast*> stored{};
};
