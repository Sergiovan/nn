#include "frontend/compiler.h"

#include "frontend/compilers/file_parser.h"
#include "frontend/compilers/ast_compiler.h"

#include "common/token.h"
#include "common/logger.h"
#include "common/util.h"
#include "common/symbol_table.h"

nnmodule::nnmodule(compiler& p, const std::string& filename) : ts{filename}, c{p} {
    namespace fs = std::filesystem;
    
    fs::path path{filename};
    
    if (fs::exists(path)) {
        abs_loc = fs::absolute(path);
    } 
    
    st = new symbol_table{};
    // Else error will be noticed by first pass
    
}

nnmodule::~nnmodule() {
    ASSERT(st, "st was nullptr");
    delete st;
}

void nnmodule::print_errors() {
    for (auto& [a, e] : errors) {
        logger::error() << e;
        token* t = a->tok;
        logger::info() << "@ " << t->line + 1 << ":" << t->column + 1 << " ~ " << token::text_between(a->get_leftmost_token(), a->get_rightmost_token());
    }
}

void nnmodule::add_dependency(nnmodule* dep) {
    std::unique_lock l{lock};
    
    dependencies.push_back(dep);
}

compiler::compiler() { // TODO give number
    
}

compiler::~compiler() {    
    for (auto& [n, m] : modules) {
        ASSERT(m, "Module was null");
        delete m;
    }
    
    if (ast_comp) {
        delete ast_comp;
    }
}

nnmodule* compiler::compile(const std::string& filename) {
    nnmodule* mod = get_or_add(filename);
    root = mod;
    root_st = mod->st;
    
    bool res = compile_file(mod);
    
    if (res) {
        logger::info() << "Parsing succeeded"; 
    }
    
    res = compile_ast(mod);
    
    if (res) {
        logger::info() << "Ast compilation succeeded";
    }
    
    return mod;
}

compiler::promise compiler::parse_file_task(nnmodule* mod) {    
    auto [_, p] = task_manager.add_task([=, this](promise s) -> bool {
        if (s->is_stopped()) { 
            return false;
        }
        file_parser pass{*this, *mod};
        if (!pass.read() || !pass.pass()) {
            mod->print_errors();
            return false;
        }
        return true;
    });
    
    return p;
}


compiler::promise compiler::parse_file_task(const std::string& filename, nnmodule* from) {
    std::lock_guard lg{lock};
    nnmodule* mod = get_or_add(filename);
    if (mod) { // Already exists, pff
        return {};
    }
    if (from) {
        from->add_dependency(mod);
    }
    
    auto [_, p] = task_manager.add_task([=, this](promise s) -> bool {
        if (s->is_stopped()) { 
            return false;
        }
        return parse_file(mod);
    });
    
    return p;
}

struct compiler::frnd {
    static void parse_ast_wrapper(void* _p) {
        parse_ast_args* pa = (parse_ast_args*) _p;
        bool res = pa->c->compile_ast(pa->node, pa->st, pa->sym);
        if (!res) {
            logger::error() << "Parsing of node " << pa->node->to_simple_string() << " failed";
            logger::error() << token::text_between(pa->node->get_leftmost_token(), pa->node->get_rightmost_token());
        }
    }
};

void compiler::compile_ast_task(ast* node, symbol_table* st, symbol* sym) {
    parse_ast_args* args = new parse_ast_args{this, node, st, sym}; // TODO This leaks
    
    fiber_manager.queue_task({compiler::frnd::parse_ast_wrapper, args});
}


nnmodule* compiler::get() {
    return root;
}

nnmodule* compiler::get(const std::string& filename) {
    if (auto it = modules.find(std::filesystem::absolute(filename)); it == modules.end()) {
        return nullptr;
    } else {
        return it->second;
    }
}

nnmodule* compiler::get_or_add(const std::string& filename) {
    if (auto it = modules.find(filename); it == modules.end()) {
        nnmodule* mod = new nnmodule{*this, filename};
        return modules[mod->abs_loc] = mod;
    } else {
        return it->second;
    }
}

bool compiler::compile_file(nnmodule* mod) {
    promise p = parse_file_task(mod);
    
    task_manager.start();
    
    return p->get();
}

bool compiler::parse_file(nnmodule* mod) {
    file_parser pass{*this, *mod};
    if (!pass.read() || !pass.pass()) {
        mod->print_errors();
        return false;
    }
    return true;
}

bool compiler::compile_ast(nnmodule* mod) {
    ast_comp = new ast_compiler{*this, *mod, mod->root};
    
    compile_ast_task(mod->root, mod->st, nullptr);
    
    return fiber_manager.run();
}

bool compiler::compile_ast(ast* node, symbol_table* st, symbol* sym) {
    ast_comp->compile_root(node, st, sym);
    return true;
}
