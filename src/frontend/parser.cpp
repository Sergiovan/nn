#include "frontend/parser.h"

#include "parser_passes/token_to_ast_pass.h"
#include "common/logger.h"
#include "common/util.h"

nnmodule::nnmodule(parser& p, const std::string& filename) : ts{filename}, p{p} {
    namespace fs = std::filesystem;
    
    fs::path path{filename};
    
    if (fs::exists(path)) {
        abs_loc = fs::absolute(path);
    } 
    
    // Else error will be noticed by first pass
    
}

pass_type nnmodule::get_current() {
    return passed;
}

pass_type nnmodule::get_target() {
    std::unique_lock{lock};
    return target_pass;
}

void nnmodule::update_current(pass_type pass) {    
    passed = pass;
}

void nnmodule::update_target(pass_type pass) {
    std::unique_lock{lock};
    
    if (passed < pass && target_pass < pass) {
        target_pass = pass;
    }
}

void nnmodule::print_errors() {
    for (auto& [t, e] : errors) {
        logger::error() << e;
        logger::info() << "@ " << *(t->tok);
    }
}

bool nnmodule::is_active() {
    std::unique_lock l{lock};
    return !!pr;
}

void nnmodule::activate(thread_pool::promise np) {
    ASSERT(!pr, "Module was already active");
    std::unique_lock l{lock};
    pr = np;
    cond.notify_all();
}

void nnmodule::deactivate() {
    ASSERT(pr, "Module was not active");
    std::unique_lock l{lock};
    pr = {nullptr};
    cond.notify_all();
}

bool nnmodule::wait_for_value() {
    std::unique_lock l{lock};
    while (!pr) {
        cond.wait(l);
    }
    auto copy = pr;
    l.unlock();
    return copy->get(); // Blocks until done
}

void nnmodule::wait_and_activate(thread_pool::promise p) {
    std::unique_lock l{lock};
    while (pr) {
        cond.wait(l);
    }
    
    pr = p;
    cond.notify_all();
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
    
    bool res = _parse(*mod);
    
    if (res) {
        logger::info() << "Parsing succeeded"; 
    }
    
    return mod;
}

nnmodule* parser::add_pass_task(nnmodule* mod, pass_type n) {
    ASSERT(mod, "Module was nullptr");    
    mod->update_target(n);
    
    pass_task(*mod);
    
    return mod;
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

bool parser::_parse(nnmodule& mod) {
    
    mod.update_target(pass_type::LAST);
    promise p = pass_task(mod);
    
    task_manager.start();
    
    return p->get();
}

parser::promise parser::pass_task(nnmodule& mod) {
    
    auto [_, p] = task_manager.add_task([=, &mod](promise s) -> bool {
        mod.wait_and_activate(s);
        return pass(mod, s);
    });
    
    return p;
}

bool parser::pass_sync(nnmodule& mod) {
    
    promise s = {};
    
    mod.wait_and_activate(s);
    return pass(mod, s);
    
}

bool parser::pass(nnmodule& mod, promise s) {
    generic_guard g{[&mod]{mod.deactivate();}};
    
    switch (mod.get_current()) {
        case pass_type::NO_PASS: { // token to ast
            if (mod.get_target() == pass_type::NO_PASS) {
                return true;
            }
            token_to_ast_pass pass1{*this, mod};
            
            logger::debug() << "Pass 0::read is a go on " << mod.ts.get_name();
            if (!pass1.read()) {
                mod.print_errors();
                mod.update_current(pass_type::ERROR);
                return false;
            }
            logger::debug() << "Pass 0::read is done on " << mod.ts.get_name();
            
            if (s->is_stopped()) {
                mod.update_current(pass_type::ERROR);
                return false;
            }
            
            logger::debug() << "Pass 0 is a go on " << mod.ts.get_name();
            if (!pass1.pass()) {
                mod.print_errors();
                mod.update_current(pass_type::ERROR);
                return false;
            }
            logger::debug() << "Pass 0 is done on " << mod.ts.get_name();
            
            mod.update_current(pass_type::TOKEN_TO_AST);
            [[fallthrough]];
        }
        case pass_type::TOKEN_TO_AST:
            if (mod.get_target() == pass_type::TOKEN_TO_AST) {
                return true;
            }
            
            if (s->is_stopped()) {
                mod.update_current(pass_type::ERROR);
                return false;
            }
            
            mod.update_current(pass_type::TYPES_AND_ST);
            [[fallthrough]];
        case pass_type::TYPES_AND_ST:
            if (mod.get_target() == pass_type::TYPES_AND_ST) {
                return true;
            }
            
            if (s->is_stopped()) {
                mod.update_current(pass_type::ERROR);
                return false;
            }
            
            mod.update_current(pass_type::CONSTANT_PROPAGATION);
            [[fallthrough]];
        case pass_type::CONSTANT_PROPAGATION:
            if (mod.get_target() == pass_type::CONSTANT_PROPAGATION) {
                return true;
            }
            
            if (s->is_stopped()) {
                mod.update_current(pass_type::ERROR);
                return false;
            }
            
            mod.update_current(pass_type::CONSTANT_PROPAGATION);
            [[fallthrough]];
        case pass_type::LAST:
            if (mod.get_target() == pass_type::LAST) {
                return true;
            }
            break;
        case pass_type::ERROR:
            return false;
    }
    
    return false;
}
