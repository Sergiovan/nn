#pragma once

#include "common/ast.h"

class ast_optimizer {
public:
    ast_optimizer();
    
    ast* optimize(ast* entry);
};
