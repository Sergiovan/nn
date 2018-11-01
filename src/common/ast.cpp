#include "common/ast.h"

#include "common/type.h"
#include "common/symbol_table.h"

using namespace Grammar;

ast_string::~ast_string() {
    if (chars) {
        delete chars;
    }
}

ast_array::~ast_array() {
    if (elems) {
        for (u64 i = 0; i < length; ++i) {
            delete elems[i];
        }
        delete elems;
    }
}

ast_struct::ast_struct(type* t) : t(t) {
    if (t) {
        u64 elems = t->as_struct().fields.size();
        ast_struct::elems = new ast*[elems];
    }
}

ast_struct::ast_struct(ast** elems, type* t) : elems(elems), t(t) {
    
}

ast_struct::~ast_struct() {
    if (elems) {
        for (u64 i = 0; i < t->as_struct().fields.size(); ++i) {
            delete elems[i];
        }
        delete elems;
    }
}

ast_closure::~ast_closure() {
    if (function) {
        delete function;
    }
    if (elems) {
        for (u64 i = 0; i < size; ++i) {
            delete elems[i];
        }
        delete elems;
    }
}

bool ast_unary::is_assignable() {
    return false; // TODO as we go
}

ast_unary::~ast_unary() {
    if (owned && node) {
        delete node;
    }
}

bool ast_binary::is_assignable() {
    return false; // TODO as we go
}

ast_binary::~ast_binary() {
    if (lowned && left) {
        delete left;
    }
    if (rowned && right) {
        delete right;
    }
}

ast_block::~ast_block() {
    for (ast* stmt : stmts) {
        if (stmt) {
            delete stmt;
        }
    }
}

ast_function::~ast_function() {
    if (block) {
        delete block;
    }
}

ast* ast::none() {
    ast* r = new ast;
    r->t = east_type::NONE;
    r->n = ast_none{};
    return r;
}

ast* ast::symbol(st_entry* sym, const std::string& str) {
    ast* r = new ast;
    r->t = east_type::SYMBOL;
    r->n = ast_symbol{sym, str};
    return r;
}

ast* ast::byte(u8 value, type* t) {
    ast* r = new ast;
    r->t = east_type::BYTE;
    r->n = ast_byte{value, t};
    return r;
}

ast* ast::word(u16 value, type* t) {
    ast* r = new ast;
    r->t = east_type::WORD;
    r->n = ast_word{value, t};
    return r;
}

ast* ast::dword(u32 value, type* t) {
    ast* r = new ast;
    r->t = east_type::DWORD;
    r->n = ast_dword{value, t};
    return r;
}

ast* ast::qword(u64 value, type* t) {
    ast* r = new ast;
    r->t = east_type::QWORD;
    r->n = ast_qword{value, t};
    return r;
}

ast* ast::string(u8* chars, u64 length) {
    ast* r = new ast;
    r->t = east_type::STRING;
    r->n = ast_string{chars, length};
    return r;
}

ast* ast::array(ast** elems, u64 length, type* t) {
    ast* r = new ast;
    r->t = east_type::ARRAY;
    r->n = ast_array{elems, length, t};
    return r;
}

ast* ast::_struct(type* t, ast** elems) {
    ast* r = new ast;
    r->t = east_type::STRUCT;
    r->n = ast_struct{elems, t};
    return r;
}

ast* ast::closure(ast* function, ast** elems, u64 size) {
    ast* r = new ast;
    r->t = east_type::CLOSURE;
    r->n = ast_closure{function, elems, size};
    return r;
}

ast* ast::unary(Symbol op, ast* node, type* t, bool post, bool owned) {
    ast* r = new ast;
    r->t = post ? east_type::POST_UNARY : east_type::PRE_UNARY;
    r->n = ast_unary{op, node, t, post, owned};
    return r;
}

ast* ast::binary(Symbol op, ast* left, ast* right, type* t, bool lowned, bool rowned) {
    ast* r = new ast;
    r->t = east_type::BINARY;
    r->n = ast_binary{op, left, right, t, lowned, rowned};
    return r;
}

ast* ast::block(symbol_table* st) {
    ast* r = new ast;
    r->t = east_type::BLOCK;
    r->n = ast_block{};
    r->as_block().st = st;
    return r;
}

ast* ast::function(ast* block, type* t) {
    ast* r = new ast;
    r->t = east_type::FUNCTION;
    r->n = ast_function{block, t};
    return r;
}

ast_none& ast::as_none() {
    return std::get<ast_none>(n);
}

ast_symbol& ast::as_symbol() {
    return std::get<ast_symbol>(n);
}

ast_byte& ast::as_byte() {
    return std::get<ast_byte>(n);
}

ast_word& ast::as_word() {
    return std::get<ast_word>(n);
}

ast_dword& ast::as_dword() {
    return std::get<ast_dword>(n);
}

ast_qword& ast::as_qword() {
    return std::get<ast_qword>(n);
}

ast_string& ast::as_string() {
    return std::get<ast_string>(n);
}

ast_array& ast::as_array() {
    return std::get<ast_array>(n);
}

ast_struct& ast::as_struct() {
    return std::get<ast_struct>(n);
}

ast_closure& ast::as_closure() {
    return std::get<ast_closure>(n);
}

ast_unary& ast::as_unary() {
    return std::get<ast_unary>(n);
}

ast_binary& ast::as_binary() {
    return std::get<ast_binary>(n);
}

ast_block& ast::as_block() {
    return std::get<ast_block>(n);
}

ast_function& ast::as_function() {
    return std::get<ast_function>(n);
}

bool ast::is_none() {
    return t == east_type::NONE;
}

bool ast::is_symbol() {
    return t == east_type::SYMBOL;
}

bool ast::is_byte() {
    return t == east_type::BYTE;
}

bool ast::is_word() {
    return t == east_type::WORD;
}

bool ast::is_dword() {
    return t == east_type::DWORD;
}

bool ast::is_qword() {
    return t == east_type::QWORD;
}

bool ast::is_string() {
    return t == east_type::STRING;
}

bool ast::is_array() {
    return t == east_type::ARRAY;
}

bool ast::is_struct() {
    return t == east_type::STRUCT;
}

bool ast::is_closure() {
    return t == east_type::CLOSURE;
}

bool ast::is_unary() {
    return t == east_type::PRE_UNARY || t == east_type::POST_UNARY;
}

bool ast::is_binary() {
    return t == east_type::BINARY;
}

bool ast::is_block() {
    return t == east_type::BLOCK;
}

bool ast::is_function() {
    return t == east_type::FUNCTION;
}

type* ast::get_type() {
    switch (t) {
        case east_type::SYMBOL: return nullptr; // TODO
        case east_type::BYTE: return as_byte().t;
        case east_type::WORD: return as_word().t;
        case east_type::DWORD: return as_dword().t;
        case east_type::QWORD: return as_qword().t;
        case east_type::STRING: return nullptr; // TODO
        case east_type::ARRAY: return as_array().t;
        case east_type::STRUCT: return as_struct().t;
        case east_type::PRE_UNARY: [[fallthrough]];
        case east_type::POST_UNARY: return as_unary().t;
        case east_type::BINARY: return as_binary().t;
        case east_type::FUNCTION: return as_function().t;
        case east_type::CLOSURE: return as_closure().function->as_function().t;
        case east_type::NONE: [[fallthrough]];
        case east_type::BLOCK: [[fallthrough]];   
        default:
            return nullptr;
    }
}

bool ast::is_assignable() {
    switch (t) {
        case east_type::PRE_UNARY: [[fallthrough]];
        case east_type::POST_UNARY: return as_unary().is_assignable();
        case east_type::BINARY: return as_binary().is_assignable();
        case east_type::SYMBOL: return true; // TODO
        case east_type::FUNCTION: [[fallthrough]];
        case east_type::CLOSURE: [[fallthrough]];   
        case east_type::BLOCK: [[fallthrough]];   
        case east_type::NONE: [[fallthrough]];
        case east_type::BYTE: [[fallthrough]];
        case east_type::WORD: [[fallthrough]];
        case east_type::DWORD: [[fallthrough]];
        case east_type::QWORD: [[fallthrough]];
        case east_type::STRING: [[fallthrough]];
        case east_type::ARRAY: [[fallthrough]];
        case east_type::STRUCT: [[fallthrough]];
        default:
            return false;
    }
}
