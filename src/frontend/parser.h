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

struct ast;
class symbol_table;

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

class parser;

class nnmodule {
public:
    nnmodule(parser& p, const std::string& filename);
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
    parser& p;
    
    std::mutex lock{};
    std::condition_variable cond{}; // Module activated or deactivated
};

class parser {
public:    
    using promise = thread_pool::promise;
    
    parser();
    ~parser();
    
    nnmodule* parse(const std::string& filename);
    promise parse_file_task(nnmodule* mod);
    promise parse_file_task(const std::string& filename, nnmodule* from);
    promise parse_ast_task(ast* node);
    
    nnmodule* get();
    nnmodule* get(const std::string& filename);
    nnmodule* get_or_add(const std::string& filename);
private:
    
    // Full parse
    bool _parse(nnmodule* mod);
    
    thread_pool task_manager{8};
    
    nnmodule* root{nullptr};
    symbol_table* root_st{nullptr};
    dict<std::string, nnmodule*> modules{};
    
    std::mutex lock{};
};
