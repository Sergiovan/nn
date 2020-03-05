#include "frontend/parser.h"

#include "parser_passes/token_to_ast_pass.h"

nnmodule::nnmodule(const std::string& filename) : ts{filename} {
    
}

parser::parser() {
    
}

parser::~parser() {
    for (auto& [n, m] : modules) {
        ASSERT(m, "Module was null");
        delete m;
    }
}

nnmodule* parser::parse(const std::string& filename) {
    nnmodule* mod = new nnmodule{filename};
    root = mod;
    
    _parse(*mod);
    
    return mod;
}

nnmodule* parser::parse_module(const std::string& filename) {
    nnmodule* mod = modules[filename];
    if (!mod) {
        mod = new nnmodule{filename};
        _parse(*mod);
    }
    
    return mod;
}

nnmodule* parser::get() {
    return root;
}

void parser::_parse(nnmodule& mod) {
    
    token_to_ast_pass pass1{mod};
    pass1.pass();
    
}
