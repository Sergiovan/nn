#pragma once

#include <stack>

#include "common/type_table.h"
#include "common/token_stream.h"
#include "common/defs.h"

struct ast;

struct module_error {
    ast* position;
    std::string message;
};

struct nnmodule {
    nnmodule(const std::string& filename);
    
    token_stream ts;
    ast* root{nullptr};
    // symbol_table st{};
    type_table tt{};
    
    std::vector<module_error> warnings{};
    std::vector<module_error> errors{};
};

class parser {
public:
    parser();
    ~parser();
    
    nnmodule* parse(const std::string& filename);
    nnmodule* parse_module(const std::string& filename);
    
    nnmodule* get();
private:
    void _parse(nnmodule& mod);
    
    nnmodule* root{nullptr};
    dict<std::string, nnmodule*> modules{};
};
