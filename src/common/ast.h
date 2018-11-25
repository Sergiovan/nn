#pragma once

#include <variant>
#include <vector>

#include "common/grammar.h"

struct ast;
struct type;
struct st_entry;
class symbol_table;


enum class east_type {
    NONE, SYMBOL, BYTE, WORD, DWORD, QWORD,
    STRING, ARRAY, STRUCT, PRE_UNARY, POST_UNARY,
    BINARY, BLOCK, FUNCTION, CLOSURE, TYPE
};

struct ast_none{};

struct ast_symbol {
    st_entry* symbol;
    std::string name;
};

struct ast_byte {
    u8 data;
    type* t;
};

struct ast_word {
    u16 data;
    type* t;
};

struct ast_dword {
    u32 data;
    type* t;
};

struct ast_qword {
    u64 data;
    type* t;
};

struct ast_string {
    u8* chars; // Owned
    u64 length;
    
    ast_string() = default;
    ~ast_string();
    
    ast_string(const ast_string& o);
    ast_string(ast_string&& o);
    ast_string& operator=(const ast_string& o);
    ast_string& operator=(ast_string&& o);
};

struct ast_array {
    ast** elems; // Owned
    u64 length;
    type* t;
    
    ast_array() = default;
    ~ast_array();
    
    ast_array(const ast_array& o);
    ast_array(ast_array&& o);
    ast_array& operator=(const ast_array& o);
    ast_array& operator=(ast_array&& o);
};

struct ast_struct {
    ast** elems; // Owned. As many elements as struct elements.
    type* t; // Struct type
    
    explicit ast_struct(type* t);
    ast_struct(ast** elems, type* t);
    ~ast_struct();
    
    ast_struct(const ast_struct& o);
    ast_struct(ast_struct&& o);
    ast_struct& operator=(const ast_struct& o);
    ast_struct& operator=(ast_struct&& o);
};

struct ast_closure {
    ast* function; // Owned. Ast of function type
    ast** elems; // Owned
    u64 size;
    
    ast_closure() = default;
    ~ast_closure();
    
    ast_closure(const ast_closure& o);
    ast_closure(ast_closure&& o);
    ast_closure& operator=(const ast_closure& o);
    ast_closure& operator=(ast_closure&& o);
};

struct ast_unary {
    Grammar::Symbol op;
    ast* node{nullptr}; // Owned?
    type* t{nullptr};
    bool post{true};
    
    bool owned{true};
    
    bool is_assignable();
    
    ast_unary() = default;
    ~ast_unary();
    
    ast_unary(const ast_unary& o);
    ast_unary(ast_unary&& o);
    ast_unary& operator=(const ast_unary& o);
    ast_unary& operator=(ast_unary&& o);
};

struct ast_binary {
    Grammar::Symbol op;
    ast* left{nullptr}; // Owned?
    ast* right{nullptr}; // Owned?
    type* t{nullptr};
    
    bool lowned{true};
    bool rowned{true};
    
    bool is_assignable();
    
    ast_binary() = default;
    ~ast_binary();
    
    ast_binary(const ast_binary& o);
    ast_binary(ast_binary&& o);
    ast_binary& operator=(const ast_binary& o);
    ast_binary& operator=(ast_binary&& o);
};

struct ast_block {
    std::vector<ast*> stmts{}; // Owned
    symbol_table* st{nullptr};
    
    ast_block() = default;
    ~ast_block();
    
    ast_block(const ast_block& o);
    ast_block(ast_block&& o);
    ast_block& operator=(const ast_block& o);
    ast_block& operator=(ast_block&& o);
};

struct ast_function {
    ast* block; // Owned
    type* t;
    
    ast_function() = default;
    ~ast_function();
    
    ast_function(const ast_function& o);
    ast_function(ast_function&& o);
    ast_function& operator=(const ast_function& o);
    ast_function& operator=(ast_function&& o);
};

struct ast_nntype {
    type* t{nullptr};
    std::vector<ast*> array_sizes{}; // Owned
    
    ast_nntype() = default;
    ~ast_nntype();
    
    ast_nntype(const ast_nntype& o);
    ast_nntype(ast_nntype&& o);
    ast_nntype& operator=(const ast_nntype& o);
    ast_nntype& operator=(ast_nntype&& o);
};

using ast_variant = std::variant<ast_none, ast_symbol, ast_byte, ast_word,
                                 ast_dword, ast_qword, ast_string, ast_array,
                                 ast_struct, ast_closure, ast_unary, ast_binary,
                                 ast_block, ast_function, ast_nntype>;

struct ast {
    east_type t;
    ast_variant n;
    
    static ast* none();
    static ast* symbol(st_entry* sym = nullptr, const std::string& str = {});
    static ast* byte(u8 value = 0, type* t = nullptr);
    static ast* word(u16 value = 0, type* t = nullptr);
    static ast* dword(u32 value = 0, type* t = nullptr);
    static ast* qword(u64 value = 0, type* t = nullptr);
    static ast* string(const std::string& str);
    static ast* string(u8* chars = nullptr, u64 length = 0);
    static ast* array(ast** elems = nullptr, u64 length = 0, type* t = nullptr);
    static ast* _struct(type* t = nullptr, ast** elems = nullptr);
    static ast* closure(ast* function = nullptr, ast** elems = nullptr, u64 size = 0);
    static ast* unary(Grammar::Symbol op = Grammar::Symbol::SYMBOL_INVALID, ast* node = nullptr, type* t = nullptr, bool post = true, bool owned = true);
    static ast* binary(Grammar::Symbol op = Grammar::Symbol::SYMBOL_INVALID, ast* left = nullptr, ast* right = nullptr, type* t = nullptr, bool lowned = true, bool rowned = true);
    static ast* block(symbol_table* st = nullptr);
    static ast* function(ast* block = nullptr, type* t = nullptr);
    static ast* nntype(type* t = nullptr);
    
    ast_none&     as_none();
    ast_symbol&   as_symbol();
    ast_byte&     as_byte();
    ast_word&     as_word();
    ast_dword&    as_dword();
    ast_qword&    as_qword();
    ast_string&   as_string();
    ast_array&    as_array();
    ast_struct&   as_struct();
    ast_closure&  as_closure();
    ast_unary&    as_unary();
    ast_binary&   as_binary();
    ast_block&    as_block();
    ast_function& as_function();
    ast_nntype&   as_nntype();
    
    bool is_none();
    bool is_symbol();
    bool is_byte();
    bool is_word();
    bool is_dword();
    bool is_qword();
    bool is_string();
    bool is_array();
    bool is_struct();
    bool is_closure();
    bool is_unary();
    bool is_binary();
    bool is_block();
    bool is_function();
    bool is_nntype();
    
    type* get_type();
    bool is_assignable();
};
