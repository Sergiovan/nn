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
    BINARY, BLOCK, FUNCTION, CLOSURE
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
    
    ~ast_string();
};

struct ast_array {
    ast** elems; // Owned
    u64 length;
    type* t;
    
    ~ast_array();
};

struct ast_struct {
    ast** elems; // Owned. As many elements as struct elements.
    type* t; // Struct type
    
    explicit ast_struct(type* t);
    ast_struct(ast** elems, type* t);
    ~ast_struct();
};

struct ast_closure {
    ast* function; // Owned
    ast** elems; // Owned
    u64 size;
    
    ~ast_closure();
};

struct ast_unary {
    Grammar::Symbol op;
    ast* node{nullptr};
    type* t{nullptr};
    bool post{true};
    
    bool owned{true};
    
    bool is_assignable();
    
    ~ast_unary();
};

struct ast_binary {
    Grammar::Symbol op;
    ast* left{nullptr}; // Owned
    ast* right{nullptr}; // Owned
    type* t{nullptr};
    
    bool lowned{true};
    bool rowned{true};
    
    bool is_assignable();
    
    ~ast_binary();
};

struct ast_block {
    std::vector<ast*> stmts{}; // Owned
    symbol_table* st{nullptr};
    
    ~ast_block();
};

struct ast_function {
    ast* block; // Owned
    type* t;
    
    ~ast_function();
};

using ast_variant = std::variant<ast_none, ast_symbol, ast_byte, ast_word,
                                 ast_dword, ast_qword, ast_string, ast_array,
                                 ast_struct, ast_closure, ast_unary, ast_binary,
                                 ast_block, ast_function>;

struct ast {
    east_type t;
    ast_variant n;
    
    static ast* none();
    static ast* symbol(st_entry* sym = nullptr, const std::string& str = {});
    static ast* byte(u8 value = 0, type* t = nullptr);
    static ast* word(u16 value = 0, type* t = nullptr);
    static ast* dword(u32 value = 0, type* t = nullptr);
    static ast* qword(u64 value = 0, type* t = nullptr);
    static ast* string(u8* chars = nullptr, u64 length = 0);
    static ast* array(ast** elems = nullptr, u64 length = 0, type* t = nullptr);
    static ast* _struct(type* t = nullptr, ast** elems = nullptr);
    static ast* closure(ast* function = nullptr, ast** elems = nullptr, u64 size = 0);
    static ast* unary(Grammar::Symbol op = Grammar::Symbol::SYMBOL_INVALID, ast* node = nullptr, type* t = nullptr, bool post = true, bool owned = true);
    static ast* binary(Grammar::Symbol op = Grammar::Symbol::SYMBOL_INVALID, ast* left = nullptr, ast* right = nullptr, type* t = nullptr, bool lowned = true, bool rowned = true);
    static ast* block(symbol_table* st = nullptr);
    static ast* function(ast* block = nullptr, type* t = nullptr);
    
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
    
    type* get_type();
    bool is_assignable();
};
