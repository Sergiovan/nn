#include "common/ast.h"

#include "common/type.h"
#include "common/symbol_table.h"
#include "common/type_table.h"

#include <cstring>
#include <sstream>
#include <iomanip>

using namespace Grammar;

std::string ast_symbol::get_name() {
    if (symbol) {
        return symbol->name;
    } else {
        return {};
    }
}


ast_string::~ast_string() {
    if (chars) {
        delete [] chars;
    }
}

ast_string::ast_string(const ast_string& o) {
    if (o.chars) {
        chars = new u8[o.length];
        std::memcpy(chars, o.chars, o.length);
    } else {
        chars = nullptr;
    }
    length = o.length;
}

ast_string::ast_string(ast_string&& o) {
    std::swap(chars, o.chars);
    std::swap(length, o.length);
}

ast_string& ast_string::operator=(const ast_string& o) {
    if (this != &o) {
        if (chars) {
            delete [] chars;
        }
        chars = new u8[o.length];
        std::memcpy(chars, o.chars, o.length);
        length = o.length;
    }
    return *this;
}

ast_string& ast_string::operator=(ast_string&& o) {
    if (this != &o) {
        std::swap(chars, o.chars);
        std::swap(length, o.length);
    }
    return *this;
}

ast_array::~ast_array() {
    if (elems) {
        for (u64 i = 0; i < length; ++i) {
            delete elems[i];
        }
        delete [] elems;
    }
}

ast_array::ast_array(const ast_array& o) {
    elems = new ast*[o.length];
    for (u64 i = 0; i < o.length; ++i) {
        if (o.elems && o.elems[i]) {
            elems[i] = new ast;
            *(elems[i]) = *(o.elems[i]);
        } else {
            elems[i] = nullptr;
        }
    }
    length = o.length;
    t = o.t;
}

ast_array::ast_array(ast_array&& o) {
    std::swap(elems, o.elems);
    std::swap(length, o.length);
    t = o.t;
}

ast_array & ast_array::operator=(const ast_array& o) {
    if (this != &o) {
        if (elems) {
            for (u64 i = 0; i < length; ++i) {
                delete elems[i];
            }
            delete [] elems;
        }
        elems = new ast*[o.length];
        for (u64 i = 0; i < o.length; ++i) {
            if (o.elems && o.elems[i]) {
                elems[i] = new ast;
                *(elems[i]) = *(o.elems[i]);
            } else {
                elems[i] = nullptr;
            }
        }
        length = o.length;
        t = o.t;
    }
    return *this;
}

ast_array & ast_array::operator=(ast_array&& o) {
    if (this != &o) {
        std::swap(elems, o.elems);
        std::swap(length, o.length);
        t = o.t;
    }
    return *this;
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
        delete [] elems;
    }
}

ast_struct::ast_struct(const ast_struct& o) {
    u64 onelems = o.t->as_struct().fields.size();
    elems = new ast*[onelems];
    for (u64 i = 0; i < onelems; ++i) {
        if (o.elems && o.elems[i]) {
            elems[i] = new ast;
            *(elems[i]) = *(o.elems[i]);
        } else {
            elems[i] = nullptr;
        }
    }
    t = o.t;
}

ast_struct::ast_struct(ast_struct && o) {
    std::swap(elems, o.elems);
    t = o.t;
}

ast_struct & ast_struct::operator=(const ast_struct& o) {
    if (this != &o) {
        u64 nelems = t->as_struct().fields.size();
        if (elems) {
            for (u64 i = 0; i < nelems; ++i) {
                delete elems[i];
            }
            delete [] elems;
        }
        u64 onelems = o.t->as_struct().fields.size();
        elems = new ast*[onelems];
        for (u64 i = 0; i < onelems; ++i) {
            if (o.elems && o.elems[i]) {
                elems[i] = new ast;
                *(elems[i]) = *(o.elems[i]);
            } else {
                elems[i] = nullptr;
            }
        }
        t = o.t;
    }
    return *this;
}

ast_struct & ast_struct::operator=(ast_struct && o) {
    if (this != &o) {
        std::swap(elems, o.elems);
        t = o.t;
    }
    return *this;
}

ast_closure::~ast_closure() {
    if (function) {
        delete function;
    }
    if (elems) {
        for (u64 i = 0; i < size; ++i) {
            delete elems[i];
        }
        delete [] elems;
    }
}

ast_closure::ast_closure(const ast_closure& o) {
    if (o.function) {
        function = new ast;
        *function = *o.function;
    } else {
        function = nullptr;
    }
    elems = new ast*[o.size];
    for (u64 i = 0; i < o.size; ++i) {
        if (o.elems && o.elems[i]) {
            elems[i] = new ast;
            *(elems[i]) = *(o.elems[i]);
        } else {
            elems[i] = nullptr;
        }
    }
    size = o.size;
}

ast_closure::ast_closure(ast_closure && o) {
    std::swap(function, o.function);
    std::swap(elems, o.elems);
    std::swap(size, o.size);
}

ast_closure & ast_closure::operator=(const ast_closure& o) {
    if (this != &o) {
        if (function) {
            delete function;
        }
        if (elems) {
            for (u64 i = 0; i < size; ++i) {
                delete elems[i];
            }
            delete [] elems;
        }
        if (o.function) {
            function = new ast;
            *function = *o.function;
        } else {
            function = nullptr;
        }
        elems = new ast*[o.size];
        for (u64 i = 0; i < o.size; ++i) {
            if (o.elems[i]) {
                elems[i] = new ast;
                *(elems[i]) = *(o.elems[i]);
            } else {
                elems[i] = nullptr;
            }
        }
        size = o.size;
    }
    return *this;
}

ast_closure & ast_closure::operator=(ast_closure && o) {
    if (this != &o) {
        std::swap(function, o.function);
        std::swap(elems, o.elems);
        std::swap(size, o.size);
    }
    return *this;
}

ast_unary::~ast_unary() {
    if (owned && node) {
        delete node;
    }
}

ast_unary::ast_unary(const ast_unary& o) {
    op = o.op;
    
    if (o.owned) {
        if (o.node) {
            node = new ast;
            *node = *o.node;
        } else {
            node = nullptr;
        }
    } else {
        node = o.node;
    }
    
    t = o.t;
    assignable = o.assignable;
    post = o.post;
    
    owned = o.owned;
}

ast_unary::ast_unary(ast_unary && o) {
    op = o.op;
    std::swap(node, o.node);
    t = o.t;
    assignable = o.assignable;
    post = o.post;
    std::swap(owned, o.owned);
}

ast_unary & ast_unary::operator=(const ast_unary& o) {
    if (this != &o) {
        if (owned && node) {
            delete node;
        }
        
        op = o.op;
        
        if (o.owned) {
            if (o.node) {
                node = new ast;
                *node = *o.node;
            } else {
                node = nullptr;
            }
        } else {
            node = o.node;
        }
        
        t = o.t;
        assignable = o.assignable;
        post = o.post;
        
        owned = o.owned;
    }
    return *this;
}

ast_unary & ast_unary::operator=(ast_unary && o) {
    if (this != &o) {
        op = o.op;
        std::swap(node, o.node);
        t = o.t;
        assignable = o.assignable;
        post = o.post;
        std::swap(owned, o.owned);
    }
    return *this;
}

ast_binary::~ast_binary() {
    if (lowned && left) {
        delete left;
    }
    if (rowned && right) {
        delete right;
    }
}

ast_binary::ast_binary(const ast_binary& o) {    
    op = o.op;
    
    if (o.lowned) {
        if (o.left) {
            left = new ast;
            *left = *o.left;
        } else {
            left = nullptr;
        }
    } else {
        left = o.left;
    }
    
    if (o.rowned) {
        if (o.right) {
            right = new ast;
            *right = *o.right;
        } else {
            right = nullptr;
        }
    } else {
        right = o.right;
    }
    
    t = o.t;
    assignable = o.assignable;
    lowned = o.lowned;
    rowned = o.rowned;
    
}

ast_binary::ast_binary(ast_binary && o) {
    op = o.op;
    std::swap(left, o.left);
    std::swap(right, o.right);
    t = o.t;
    assignable = o.assignable;
    std::swap(lowned, o.lowned);
    std::swap(rowned, o.rowned);
}

ast_binary & ast_binary::operator=(const ast_binary& o) {
    if (this != &o) {
        if (lowned && left) {
            delete left;
        }
        if (rowned && right) {
            delete right;
        }
        
        op = o.op;
        
        if (o.lowned) {
            if (o.left) {
                left = new ast;
                *left = *o.left;
            } else {
                left = nullptr;
            }
        } else {
            left = o.left;
        }
        
        if (o.rowned) {
            if (o.left) {
                right = new ast;
                *right = *o.right;
            } else {
                right = nullptr;
            }
        } else {
            right = o.right;
        }
        
        t = o.t;
        assignable = o.assignable;
        lowned = o.lowned;
        rowned = o.rowned;
    }
    return *this;
}

ast_binary & ast_binary::operator=(ast_binary && o) {
    if (this != &o) {
        op = o.op;
        std::swap(left, o.left);
        std::swap(right, o.right);
        t = o.t;
        assignable = o.assignable;
        std::swap(lowned, o.lowned);
        std::swap(rowned, o.rowned);
    }
    return *this;
}

ast_block::~ast_block() {
    for (ast* stmt : stmts) {
        if (stmt) {
            delete stmt;
        }
    }
}

ast_block::ast_block(const ast_block& o) {
    stmts.resize(o.stmts.size());
    for (u64 i = 0; i < stmts.size(); ++i) {
        if (o.stmts[i]) {
            stmts[i] = new ast;
            *(stmts[i]) = *(o.stmts[i]);
        } else {
            stmts[i] = nullptr;
        }
    }
    
    st = o.st;
}

ast_block::ast_block(ast_block && o) {
    std::swap(stmts, o.stmts);
    st = o.st;
}

ast_block & ast_block::operator=(const ast_block& o) {
    if (this != &o) {
        for (ast* stmt : stmts) {
            if (stmt) {
                delete stmt;
            }
        }
        
        stmts.resize(o.stmts.size());
        for (u64 i = 0; i < stmts.size(); ++i) {
            if (o.stmts[i]) {
                stmts[i] = new ast;
                *(stmts[i]) = *(o.stmts[i]);
            } else {
                stmts[i] = nullptr;
            }
        }
        
        st = o.st;
    }
    return *this;
}

ast_block & ast_block::operator=(ast_block && o) {
    if (this != &o) {
        std::swap(stmts, o.stmts);
        st = o.st;
    }
    return *this;
}

ast_function::~ast_function() {
    if (block) {
        delete block;
    }
}

ast_function::ast_function(const ast_function& o) {
    if (o.block) {
        block = new ast;
        *block = *o.block;
    } else {
        block = nullptr;
    }
    
    t = o.t;
}

ast_function::ast_function(ast_function&& o) {
    std::swap(block, o.block);
    t = o.t;
}

ast_function& ast_function::operator=(const ast_function& o) {
    if (this != &o) {
        if (block) {
            delete block;
        }
        
        if (o.block) {
            block = new ast;
            *block = *o.block;
        } else {
            block = nullptr;
        }
        
        t = o.t;
    }
    return *this;
}

ast_function& ast_function::operator=(ast_function&& o) {
    if (this != &o) {
        std::swap(block, o.block);
        t = o.t;
    }
    return *this;
}

ast_nntype::~ast_nntype() {
    for (ast* size : array_sizes) {
        if (size) {
            delete size;
        }
    }
}

ast_nntype::ast_nntype(const ast_nntype& o) {
    t = o.t;
    array_sizes.resize(o.array_sizes.size());
    for (u64 i = 0; i < array_sizes.size(); ++i) {
        if (o.array_sizes[i]) {
            array_sizes[i] = new ast;
            *(array_sizes[i]) = *(o.array_sizes[i]);
        } else {
            array_sizes[i] = nullptr;
        }
    }
}

ast_nntype::ast_nntype(ast_nntype && o) {
    t = o.t;
    std::swap(array_sizes, o.array_sizes);
}

ast_nntype & ast_nntype::operator=(const ast_nntype& o) {
    if (this != &o) {
        for (ast* size : array_sizes) {
            if (size) {
                delete size;
            }
        }
        t = o.t;
        array_sizes.resize(o.array_sizes.size());
        for (u64 i = 0; i < array_sizes.size(); ++i) {
            if (o.array_sizes[i]) {
                array_sizes[i] = new ast;
                *(array_sizes[i]) = *(o.array_sizes[i]);
            } else {
                array_sizes[i] = nullptr;
            }
        }
    }
    return *this;
}

ast_nntype & ast_nntype::operator=(ast_nntype && o) {
    if (this != &o) {
        t = o.t;
        std::swap(array_sizes, o.array_sizes);
    }
    return *this;
}

ast* ast::none() {
    ast* r = new ast;
    r->t = east_type::NONE;
    r->n = ast_none{};
    return r;
}

ast* ast::symbol(st_entry* sym) {
    ast* r = new ast;
    r->t = east_type::SYMBOL;
    r->n = ast_symbol{sym};
    return r;
}

ast* ast::byte(u8 value, type* t) {
    ast* r = new ast;
    r->t = east_type::BYTE;
    r->n = ast_byte{value, t ? t : type_table::t_void};
    return r;
}

ast* ast::word(u16 value, type* t) {
    ast* r = new ast;
    r->t = east_type::WORD;
    r->n = ast_word{value, t ? t : type_table::t_void};
    return r;
}

ast* ast::dword(u32 value, type* t) {
    ast* r = new ast;
    r->t = east_type::DWORD;
    r->n = ast_dword{value, t ? t : type_table::t_void};
    return r;
}

ast* ast::qword(u64 value, type* t) {
    ast* r = new ast;
    r->t = east_type::QWORD;
    r->n = ast_qword{value, t ? t : type_table::t_void};
    return r;
}

ast* ast::string(const std::string& str) {
    ast* r = new ast;
    r->t = east_type::STRING;
    u8* chars = new u8[str.length()];
    std::memcpy(chars, str.data(), str.length());
    ast_string ss{};
    ss.chars = chars;
    ss.length = str.length();
    r->n = std::move(ss);
    return r;
}

ast* ast::string(u8* chars, u64 length) {
    ast* r = new ast;
    r->t = east_type::STRING;
    ast_string ss{};
    ss.chars = chars;
    ss.length = length;
    r->n = std::move(ss);
    return r;
}

ast* ast::array(ast** elems, u64 length, type* t) {
    ast* r = new ast;
    r->t = east_type::ARRAY;
    ast_array as{};
    as.elems = elems;
    as.length = length;
    as.t = t ? t : type_table::t_void;;
    r->n = std::move(as);
    return r;
}

ast* ast::_struct(type* t, ast** elems) {
    ast* r = new ast;
    r->t = east_type::STRUCT;
    r->n = ast_struct{elems, t ? t : type_table::t_void};
    return r;
}

ast* ast::closure(ast* function, ast** elems, u64 size) {
    ast* r = new ast;
    r->t = east_type::CLOSURE;
    ast_closure cs{};
    cs.function = function;
    cs.elems = elems;
    cs.size = size;
    r->n = std::move(cs);
    return r;
}

ast* ast::unary(Symbol op, ast* node, type* t, bool assignable, bool post, bool owned) {
    ast* r = new ast;
    r->t = post ? east_type::POST_UNARY : east_type::PRE_UNARY;
    ast_unary us{};
    us.op = op;
    us.node = node;
    us.t = t ? t : type_table::t_void;
    us.assignable = assignable;
    us.post = post;
    us.owned = owned;
    r->n = std::move(us);
    return r;
}

ast* ast::binary(Symbol op, ast* left, ast* right, type* t, bool assignable, bool lowned, bool rowned) {
    ast_binary bs{};
    bs.op = op;
    bs.left = left;
    bs.right = right;
    bs.t = t ? t : type_table::t_void;
    bs.assignable = assignable;
    bs.lowned = lowned;
    bs.rowned = rowned;
    ast* r = new ast{east_type::BINARY, std::move(bs)};
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
    ast_function fs{};
    fs.block = block;
    fs.t = t ? t : type_table::t_void;;
    r->n = std::move(fs);
    return r;
}

ast* ast::nntype(type* t) {
    ast* r = new ast;
    r->t = east_type::TYPE;
    ast_nntype ts{};
    ts.t = t ? t : type_table::t_void;;
    r->n = std::move(ts);
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

ast_nntype& ast::as_nntype() {
    return std::get<ast_nntype>(n);
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

bool ast::is_nntype() {
    return t == east_type::TYPE;
}

bool ast::is_value() {
    switch (t) {
        case east_type::NONE: [[fallthrough]];
        case east_type::BYTE: [[fallthrough]];
        case east_type::WORD: [[fallthrough]];
        case east_type::DWORD: [[fallthrough]];
        case east_type::QWORD: [[fallthrough]];
        case east_type::STRING: [[fallthrough]];
        case east_type::ARRAY: [[fallthrough]];
        case east_type::CLOSURE: [[fallthrough]];   
        case east_type::STRUCT: return true;
        case east_type::PRE_UNARY: [[fallthrough]];
        case east_type::POST_UNARY: [[fallthrough]];
        case east_type::BINARY: [[fallthrough]];
        case east_type::SYMBOL: [[fallthrough]];
        case east_type::BLOCK: [[fallthrough]];   
        case east_type::TYPE: [[fallthrough]];
        case east_type::FUNCTION: [[fallthrough]];
        default:
            return false;
    }
}

type* ast::get_type() {
    switch (t) {
        case east_type::SYMBOL: return as_symbol().symbol->get_type(); 
        case east_type::BYTE: return as_byte().t;
        case east_type::WORD: return as_word().t;
        case east_type::DWORD: return as_dword().t;
        case east_type::QWORD: return as_qword().t;
        case east_type::STRING: return type_table::t_string;
        case east_type::ARRAY: return as_array().t;
        case east_type::STRUCT: return as_struct().t;
        case east_type::PRE_UNARY: [[fallthrough]];
        case east_type::POST_UNARY: return as_unary().t;
        case east_type::BINARY: return as_binary().t;
        case east_type::FUNCTION: return as_function().t;
        case east_type::CLOSURE: return as_closure().function->as_function().t;
        case east_type::TYPE: return as_nntype().t;
        case east_type::NONE: return type_table::t_nothing;
        case east_type::BLOCK: [[fallthrough]];   
        default:
            return type_table::t_void;
    }
}

bool ast::is_assignable() {
    switch (t) {
        case east_type::PRE_UNARY: [[fallthrough]];
        case east_type::POST_UNARY: return as_unary().assignable;
        case east_type::BINARY: return as_binary().assignable;
        case east_type::SYMBOL: return as_symbol().symbol->is_field() || as_symbol().symbol->is_variable(); // Is this work?
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
        case east_type::TYPE: [[fallthrough]];
        default:
            return false;
    }
}

std::string ast::print(u64 depth, const std::string& prev) {
    using namespace std::string_literals;
    std::stringstream ss{};
    std::string sep(depth * 2, ' ');
    /* for (char c : prev) {
        switch (c) {
            case '\xb3': [[fallthrough]]; // │
            case ' ':
                sep += c; 
                break;
            case '\xc4': // ─
                if (sep.back() != ' ') {
                    sep += c;
                } else {
                    sep += ' ';
                }
                break;
            case '\xc0': // └
                sep += ' ';
                break;
            case '\xbf': // ┐
                sep += '\xc0';
                break;
            case '': // ├
                sep += '\xb3';
                break;
            case '\xda': // ┌
                sep += '\xc3';
                break;
        }
    } */
    ss << sep;
    switch (t) {
        case east_type::PRE_UNARY: 
            ss << "PRE_"; 
            [[fallthrough]];
        case east_type::POST_UNARY: { 
            ast_unary& un = as_unary();
            ss << "UNARY " << Grammar::symbol_names.at(un.op) << " (" << un.t->print(true) << ") ";
            if (un.assignable) {
                ss << "[=] ";
            }
            if (!un.owned) {
                ss << "!OWNED ";
            }
            ss << "\n";
            if (un.node) {
                sep += "\xbf\xc4"s;
                ss << un.node->print(depth + 1, sep);
            } else {
                ss << sep << "  NULLPTR\n";
            }
            return ss.str();
        }
        case east_type::BINARY: {
            ast_binary& bin = as_binary();
            ss << "BINARY " << Grammar::symbol_names.at(bin.op) << " (" << bin.t->print(true) << ") ";
            if (bin.assignable) {
                ss << "[=] ";
            }
            if (!bin.lowned) {
                ss << "!!LOWNED ";
            }
            if (!bin.rowned) {
                ss << "!!ROWNED ";
            }
            ss << "\n";
            sep += "\xda\xc4"s;
            if (bin.left) {
                ss << bin.left->print(depth + 1, sep);
            } else {
                ss << sep << "  NULLPTR\n";
            }
            *(sep.end() - 2) = '\xbf';
            if (bin.right) {
                ss << bin.right->print(depth + 1, sep);
            } else {
                ss << sep << "  NULLPTR\n";
            }
            return ss.str();
        }
        case east_type::SYMBOL: {
            ast_symbol& sym = as_symbol();
            ss << "SYMBOL " << sym.get_name() << " (" << sym.symbol->get_type()->print(true) << ") ";
            switch (sym.symbol->t) {
                case est_entry_type::FIELD: 
                    ss << "FIELD\n"; 
                    break;
                case est_entry_type::FUNCTION: {
                    ss << "FUNCTION\n";
                    break;
                }
                case est_entry_type::TYPE: 
                    ss << "TYPE\n"; 
                    break;
                case est_entry_type::VARIABLE: 
                    ss << "VARIABLE\n"; 
                    break;
                case est_entry_type::MODULE: 
                    ss << "MODULE\n"; 
                    break;
                case est_entry_type::NAMESPACE: 
                    ss << "NAMESPACE\n"; 
                    break;
                case est_entry_type::LABEL:
                    ss << "LABEL\n";
                    break;
            }
            return ss.str();
        }
        case east_type::FUNCTION: {
            ast_function& fun = as_function();
            ss << "FUNCTION (" << fun.t->print(true) << ")\n";
            sep += "\xbf\xc4"s;
            if (fun.block) {
                ss << fun.block->print(depth + 1, sep);
            } else {
                ss << sep << "  NULLPTR\n";
            }
            return ss.str();
        }
        case east_type::CLOSURE: {
            ast_closure& cls = as_closure();
            ss << "CLOSURE (" << cls.function->get_type()->print(true) << ") [" << cls.size << "]\n";
            sep += "\xda\xc4"s;
            ss << cls.function->print(depth + 1, sep);
            for (u64 i = 0; i < cls.size; ++i) {
                if (i == cls.size - 1) {
                    *(sep.end() - 2) = '\xbf';
                }
                if (cls.elems[i]) {
                    ss << cls.elems[i]->print(depth + 1, sep);
                } else {
                    ss << "NULLPTR\n";
                }
            }
            return ss.str();
        }
        case east_type::BLOCK: {
            ast_block& blk = as_block();
            ss << "BLOCK [" << blk.stmts.size() << "]\n";
            sep += "\xda\xc4"s;
            for (auto& stmt : blk.stmts) {
                if (stmt == blk.stmts.back()) {
                    *(sep.end() - 2) = '\xbf';
                }
                if (stmt) {
                ss << stmt->print(depth + 1, prev);
                } else {
                    ss << "NULLPTR\n";
                }
            }
            return ss.str();
        }
        case east_type::NONE: 
            ss << "NONE\n"; 
            return ss.str();
        case east_type::BYTE: {
            ast_byte& byt = as_byte();
            ss << "BYTE (" << byt.t->print(true) << ") " << std::hex << (u16) byt.data << "\n";
            return ss.str();
        }
        case east_type::WORD: {
            ast_word& wrd = as_word();
            ss << "WORD (" << wrd.t->print(true) << ") " << std::hex << wrd.data << "\n";
            return ss.str();
        }
        case east_type::DWORD: {
            ast_dword& dwr = as_dword();
            ss << "DWORD (" << dwr.t->print(true) << ") " << std::hex << dwr.data << "\n";
            return ss.str();
        }
        case east_type::QWORD: {
            ast_qword& qwr = as_qword();
            ss << "QWORD (" << qwr.t->print(true) << ") " << std::hex << qwr.data << "\n";
            return ss.str();
        }
        case east_type::STRING: {
            ast_string& str = as_string();
            ss << "STRING [" << str.length << "]\n";
            return ss.str();
        }
        case east_type::ARRAY: {
            ast_array& arr = as_array();
            ss << "ARRAY (" << arr.t->print(true) << ")[" << arr.length << "]\n";
            sep += "\xda\xc4"s;
            for (u64 i = 0; i < arr.length; ++i) {
                if (i == arr.length - 1) {
                    *(sep.end() - 2) = '\xbf';
                }
                if (arr.elems[i]) {
                    ss << arr.elems[i]->print(depth + 1, sep);
                } else {
                    ss << sep << "  NULLPTR\n";
                }
            }
            return ss.str();
        }
        case east_type::STRUCT: {
            ast_struct& stc = as_struct();
            ss << "STRUCT (" << stc.t->print(true) << ")\n";
            sep += "\xda\xc4"s;
            u64 length = stc.t->as_struct().fields.size();
            for (u64 i = 0; i < length; ++i) {
                if (i == length - 1) {
                    *(sep.end() - 2) = '\xbf';
                }
                if (stc.elems[i]) {
                    ss << stc.elems[i]->print(depth + 1, sep);
                } else {
                    ss << sep << "  NULLPTR\n";
                }
            }
            return ss.str();
        }
        case east_type::TYPE: {
            ast_nntype& typ = as_nntype();
            ss << "TYPE (" << typ.t->print(true) << ")\n";
            sep += "\xda\xc4"s;
            for (auto& size : typ.array_sizes) {
                if (size == typ.array_sizes.back()) {
                    *(sep.end() - 2) = '\xbf';
                }
                if (size) {
                    ss << size->print(depth + 1, prev);
                } else {
                    ss << sep << "  NULLPTR\n";
                }
            }
            return ss.str();
        }
        default:
            ss << sep << "ILLEGAL SHENANIGANS\n";
            return ss.str();
    }
    
}
