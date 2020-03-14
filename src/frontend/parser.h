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

enum class pass_type {
    ERROR, 
    NO_PASS,
    TOKEN_TO_AST,
    TYPES_AND_ST,
    CONSTANT_PROPAGATION,
    
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
    
    token_stream ts;
    ast* root{nullptr};
    // symbol_table st{};
    type_table tt{};
    std::filesystem::path abs_loc{};
    
    std::vector<module_error> warnings{};
    std::vector<module_error> errors{};
    std::vector<nnmodule*> dependencies{};
    
    pass_type get_current();
    pass_type get_target();
    void update_current(pass_type pass);
    void update_target(pass_type pass);
    void print_errors();
    
    bool is_active();
    void activate(thread_pool::promise p);
    void deactivate();
    bool wait_for_value();
    void wait_and_activate(thread_pool::promise p);
    
    void add_dependency(nnmodule* dep);
private:
    parser& p;
    
    pass_type target_pass{pass_type::NO_PASS};
    pass_type passed{pass_type::NO_PASS};
    thread_pool::promise pr{nullptr};
    
    std::mutex lock{};
    std::condition_variable cond{}; // Module activated or deactivated
};

class parser {
public:    
    parser();
    ~parser();
    
    nnmodule* parse(const std::string& filename);
    nnmodule* add_pass_task(nnmodule* mod, pass_type n);
    
    nnmodule* get();
    nnmodule* get(const std::string& filename);
    nnmodule* get_or_add(const std::string& filename);
private:
    using promise = thread_pool::promise;
    
    // Full parse
    bool _parse(nnmodule& mod);
    // Individual passes
    promise pass_task(nnmodule& mod);
    
    bool pass_sync(nnmodule& mod);
    bool pass(nnmodule& mod, promise s);
    
    thread_pool task_manager{8};
    
    nnmodule* root{nullptr};
    dict<std::string, nnmodule*> modules{};
    
    static constexpr u64 pass_amount{(u64) pass_type::LAST};
};
