#include "frontend/parser.h"

#include "parser_passes/token_to_ast_pass.h"

#include "common/logger.h"
#include "common/util.h"
#include "common/symbol_table.h"

nnmodule::nnmodule(parser& p, const std::string& filename) : ts{filename}, p{p} {
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
    for (auto& [t, e] : errors) {
        logger::error() << e;
        logger::info() << "@ " << *(t->tok);
    }
}

void nnmodule::add_dependency(nnmodule* dep) {
    std::unique_lock l{lock};
    
    dependencies.push_back(dep);
}

parser::parser() { // TODO give number
    
}

parser::~parser() {    
    for (auto& [n, m] : modules) {
        ASSERT(m, "Module was null");
        delete m;
    }
}

nnmodule* parser::parse(const std::string& filename) {
    nnmodule* mod = get_or_add(filename);
    root = mod;
    root_st = mod->st;
    
    bool res = _parse(mod);
    
    if (res) {
        logger::info() << "Parsing succeeded"; 
    }
    
    return mod;
}

parser::promise parser::parse_file_task(nnmodule* mod) {    
    auto [_, p] = task_manager.add_task([=](promise s) -> bool {
        if (s->is_stopped()) { 
            return false;
        }
        token_to_ast_pass pass{*this, *mod};
        if (!pass.read() || !pass.pass()) {
            mod->print_errors();
            return false;
        }
        return true;
    });
    
    return p;
}


parser::promise parser::parse_file_task(const std::string& filename, nnmodule* from) {
    std::lock_guard lg{lock};
    nnmodule* mod = get_or_add(filename);
    if (mod) { // Already exists, pff
        return {};
    }
    if (from) {
        from->add_dependency(mod);
    }
    
    auto [_, p] = task_manager.add_task([=](promise s) -> bool {
        if (s->is_stopped()) { 
            return false;
        }
        token_to_ast_pass pass{*this, *mod};
        if (!pass.read() || !pass.pass()) {
            mod->print_errors();
            return false;
        }
        return true;
    });
    
    return p;
}

parser::promise parser::parse_ast_task(ast* node) {
    (void) node;
    return {};
}


nnmodule* parser::get() {
    return root;
}

nnmodule* parser::get(const std::string& filename) {
    if (auto it = modules.find(std::filesystem::absolute(filename)); it == modules.end()) {
        return nullptr;
    } else {
        return it->second;
    }
}

nnmodule* parser::get_or_add(const std::string& filename) {
    if (auto it = modules.find(filename); it == modules.end()) {
        nnmodule* mod = new nnmodule{*this, filename};
        return modules[mod->abs_loc] = mod;
    } else {
        return it->second;
    }
}

bool parser::_parse(nnmodule* mod) {
    
    promise p = parse_file_task(mod);
    
    task_manager.start();
    
    return p->get();
}
