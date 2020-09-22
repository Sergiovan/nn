#pragma once

#include <functional>
#include <stack>
#include <memory>
#include <utility>
#include <filesystem>

#include "common/type_table.h"
#include "common/token_stream.h"
#include "common/defs.h"

#include "common/threading/thread_pool.h"
#include "common/fibers/fiber_pool.h"

struct ast;
struct symbol;
class symbol_table;
class ast_compiler;

enum class pass_type {
    ERROR, 
    NO_PASS,
    
    FILE_TO_AST,
    
    AST_TO_LLVM,
    AST_TO_IR,
    
    LAST
};

struct module_error {
    ast* position;
    std::string message;
};

class compiler;

class nnmodule {
public:
    nnmodule(compiler& p, const std::string& filename);
    ~nnmodule();
    
    token_stream ts;
    ast* root{nullptr};
    symbol_table* st{};
    type_table tt{};
    std::filesystem::path abs_loc{};
    
    std::vector<module_error> warnings{};
    std::vector<module_error> errors{};
    std::vector<nnmodule*> dependencies{};

    void print_errors();
    
    void add_dependency(nnmodule* dep);
private:
    compiler& c;
    
    std::mutex lock{};
    std::condition_variable cond{}; // Module activated or deactivated
};

class compiler {
public:    
    using promise = thread_pool::promise;
    
    struct parse_ast_args {
        compiler* c;
        nnmodule* mod;
        ast* node;
        symbol_table* st;
        symbol* sym;
    };
    
    compiler();
    ~compiler();
    
    nnmodule* compile(const std::string& filename);
    promise parse_file_task(nnmodule* mod);
    promise parse_file_task(const std::string& filename, nnmodule* from);
    void compile_ast_task(nnmodule* mod, ast* node, symbol_table* st, symbol* sym);
    
    nnmodule* get();
    nnmodule* get(const std::string& filename);
    nnmodule* get_or_add(const std::string& filename);
private:
    // Full compile
    bool compile_file(nnmodule* mod);
    bool parse_file(nnmodule* mod);
    bool compile_ast(nnmodule* mod);
    bool compile_ast(nnmodule* mod, ast* node, symbol_table* st, symbol* sym);
    
    thread_pool task_manager{8};
    fiber_pool fiber_manager{};
    
    nnmodule* root{nullptr};
    symbol_table* root_st{nullptr};
    dict<std::string, nnmodule*> modules{};
    
    std::mutex lock{};
    
    struct frnd;
    friend frnd;
};
