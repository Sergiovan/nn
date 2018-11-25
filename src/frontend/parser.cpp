#include "frontend/parser.h"

#include <algorithm>
#include <cstring>
#include <sstream>

#include "common/ast.h"
#include "frontend/lexer.h"
#include "frontend/reader.h"
#include "common/symbol_table.h"
#include "common/utils.h"

using namespace Grammar;

ctx_guard::ctx_guard(parser& p) : p(p) {
    
}

ctx_guard::~ctx_guard() {
    deactivate();
}


void ctx_guard::deactivate() {
    if (active) {
        p.pop_context();
        active = false;
    }
}

ast* parser::parse(lexer* l) {
    contexts.push({root_st, nullptr, nullptr, nullptr, nullptr});
    parser::l = l;
    c = l->next();
    return program();
}

ast* parser::parse(const std::string& str, bool is_file) {
    reader* r = (is_file ? reader::from_file : reader::from_string)(str);
    lexer* l = new lexer{r};
    ast* ret = parse(l);
    delete l;
    delete r;
    return ret;
}

symbol_table* parser::get_as_module() {
    return nullptr;
}

parser parser::fork() {
    return parser{}; // TODO Actually implement
}

void parser::print_errors() {
    return;
}

context& parser::ctx() {
    return contexts.top();
}

symbol_table* parser::st() {
    return contexts.top().st;
}

context& parser::push_context(bool clear) {
    contexts.emplace(clear ? context{} : ctx());
    return contexts.top();
}

context parser::pop_context() {
    context top = contexts.top();
    contexts.pop();
    return top;    
}

ctx_guard parser::guard() {
    return {*this};
}

ast* parser::error(const std::string& msg, epanic_mode mode, token* t) {
    errors.emplace_back(t ? *t : c, msg);
    if (mode != epanic_mode::NO_PANIC) {
        parser::panic(mode);
    }
    return nullptr; // In case we want to make an error AST or stub
}

void parser::panic(epanic_mode mode) {
    switch(mode) {
        case epanic_mode::SEMICOLON: {
            skip_until(Symbol::SEMICOLON);
        }
        break;
        case epanic_mode::COMMA: {
            skip_until(Symbol::COMMA);
        }
        break;
        case epanic_mode::ESCAPE_BRACE: 
            skip_until(Symbol::BRACE_RIGHT);
            break;
        case epanic_mode::ESCAPE_BRACKET:
            skip_until(Symbol::BRACKET_RIGHT);
            break;
        case epanic_mode::ESCAPE_PAREN:
            skip_until(Symbol::PAREN_RIGHT);
            break;
        case epanic_mode::IN_ARRAY:
            skip_until({Symbol::BRACKET_RIGHT, Symbol::COMMA});
            break;
        case epanic_mode::IN_STRUCT_LIT:
            skip_until({Symbol::BRACE_RIGHT, Symbol::COMMA});
            break;
        case epanic_mode::NO_PANIC:
            break; // Calm down, jeez
        case epanic_mode::ULTRA_PANIC:
            throw parser_exception{};
        default:
            throw parser_exception{};
    } 
}

token parser::next() {
    token tmp = c;
    c = l->next();
    return tmp;
}

bool parser::is(Grammar::TokenType tt) {
    return c.type == tt;
}

bool parser::is(Grammar::Symbol sym) {
    return c.type == Grammar::TokenType::SYMBOL && c.as_symbol() == sym;
}

bool parser::is(Grammar::Keyword kw) {
    return c.type == Grammar::TokenType::KEYWORD && c.as_keyword() == kw;
}

bool parser::peek(Grammar::TokenType tt, u64 lookahead) {
    token t = l->peek(lookahead);
    return t.type == tt;
}

bool parser::peek(Grammar::Symbol sym, u64 lookahead) {
    token t = l->peek(lookahead);
    return t.type == Grammar::TokenType::SYMBOL && t.as_symbol() == sym;
}

bool parser::peek(Grammar::Keyword kw, u64 lookahead) {
    token t = l->peek(lookahead);
    return t.type == Grammar::TokenType::KEYWORD && t.as_keyword() == kw;
}

bool parser::require(Grammar::TokenType tt, epanic_mode mode, const std::string& err) {
    if (!is(tt)) {
        if (err.empty()) {
            std::stringstream ss{};
            ss << c.get_info() << " - Expected \"" << Grammar::tokentype_names.at(tt) << " but got \"" << c.value << "\" instead,";
            error(ss.str(), mode);
        } else {
            error(err, mode);
        }
        return false;
    }
    return true;
}

bool parser::require(Grammar::Symbol sym, epanic_mode mode, const std::string& err) {
    if (!is(sym)) {
        if (err.empty()) {
            std::stringstream ss{};
            ss << c.get_info() << " - Expected symbol \"" << Grammar::symbol_names.at(sym) << " but got \"" << c.value << "\" instead";
            error(ss.str(), mode);
        } else {
            error(err, mode);
        }
        return false;
    }
    return true;
}

bool parser::require(Grammar::Keyword kw, epanic_mode mode, const std::string& err) {
    if (!is(kw)) {
        if (err.empty()) {
            std::stringstream ss{};
            ss << c.get_info() << " - Expected keyword \"" << Grammar::keyword_names.at(kw) << " but got \"" << c.value << "\" instead";
            error(ss.str(), mode);
        } else {
            error(err, mode);
        }
        return false;
    }
    return true;
}

void parser::compiler_assert(TokenType tt) {
    if constexpr(__debug) { // TODO Replace with DEBUG/
        if (!is(tt)) {
            throw parser_exception{};
        }
    }
}

void parser::compiler_assert(Symbol sym) {
    if constexpr(__debug) { // TODO Replace with DEBUG/
        if (!is(sym)) {
            throw parser_exception{};
        }
    }
}

void parser::compiler_assert(Keyword kw) {
    if constexpr(__debug) { // TODO Replace with DEBUG/
        if (!is(kw)) {
            throw parser_exception{};
        }
    }
}

token parser::skip(u64 amount) {
    l->skip(amount);
    return next();
}

bool parser::can_peek_skip_groups (u64 from) {
    return peek(Symbol::PAREN_LEFT, from) && !peek(Symbol::BRACE_LEFT, from) && !peek(Symbol::BRACKET_LEFT, from);
}

u64 parser::peek_skip_groups(u64 from) {
    if (!can_peek_skip_groups(from)) {
        return 0;
    }
    std::stack<Symbol> syms{};
    u64 ahead = from;
    token t = l->peek(ahead++);
    syms.push(t.as_symbol());
    while (!syms.empty()) {
        t = l->peek(ahead++);
        if (t.type == TokenType::END_OF_FILE) {
            break;
        } else if (t.type == TokenType::SYMBOL) {
            if (t.as_symbol() == Symbol::PAREN_RIGHT && syms.top() == Symbol::PAREN_LEFT) {
                syms.pop();
            } else if (t.as_symbol() == Symbol::BRACE_RIGHT && syms.top() == Symbol::BRACE_LEFT) {
                syms.pop();
            } else if (t.as_symbol() == Symbol::BRACKET_RIGHT && syms.top() == Symbol::BRACKET_LEFT) {
                syms.pop();
            } else if (t.as_symbol() == Symbol::PAREN_LEFT) {
                syms.push(t.as_symbol());
            } else if (t.as_symbol() == Symbol::BRACE_LEFT) {
                syms.push(t.as_symbol());
            } else if (t.as_symbol() == Symbol::BRACKET_LEFT) {
                syms.push(t.as_symbol());
            }
        }
    }
    return ahead - from;
}

token parser::skip_until(TokenType tt, bool skip_groups) {
    u64 amount = 0;
    
    token t = l->peek(amount);
    while (t.type != tt && t.type != TokenType::END_OF_FILE) {
        if (skip_groups) {
            u64 skip_amount = peek_skip_groups(amount);
            if (skip_amount) {
                amount += skip_amount;
                t = l->peek(amount);
                continue;
            }
        }
        t = l->peek(++amount);
    }
    return skip(amount);
}

token parser::skip_until(Symbol sym, bool skip_groups) {
    u64 amount = 0;
    
    token t = l->peek(amount);
    while (t.type != TokenType::END_OF_FILE && t.as_symbol() != sym) {
        if (skip_groups) {
            u64 skip_amount = peek_skip_groups(amount);
            if (skip_amount) {
                amount += skip_amount;
                t = l->peek(amount);
                continue;
            }
        }
        t = l->peek(++amount);
    }
    return skip(amount);
}

token parser::skip_until(const std::vector<Grammar::Symbol>& syms, bool skip_groups) {
    if (syms.empty()) {
        throw parser_exception{};
    }
    u64 amount = 0;
    
    token t = l->peek(amount);
    while (t.type != TokenType::END_OF_FILE && 
          (t.as_symbol() == Symbol::SYMBOL_INVALID || std::find(syms.begin(), syms.end(),  t.as_symbol()) != syms.end())) {
        if (skip_groups) {
            u64 skip_amount = peek_skip_groups(amount);
            if (skip_amount) {
                amount += skip_amount;
                t = l->peek(amount);
                continue;
            }
        }
        t = l->peek(++amount);
    }
    return skip(amount);
}

token parser::skip_until(Keyword kw, bool skip_groups) {
    u64 amount = 0;
    
    token t = l->peek(amount);
    while (t.type != TokenType::END_OF_FILE && t.as_keyword() != kw) {
        if (skip_groups) {
            u64 skip_amount = peek_skip_groups(amount);
            if (skip_amount) {
                amount += skip_amount;
                t = l->peek(amount);
                continue;
            }
        }
        t = l->peek(++amount);
    }
    return skip(amount);
}

ast* parser::iden(bool withthis) {
    if (!is(Keyword::THIS)) {
        require(TokenType::IDENTIFIER);
    }
    
    auto tok = next(); // iden
    auto sym = ctx().st->get(tok.value);
    if (!sym && withthis) {
        sym = ctx().st->get("this");
        if (sym && sym->is_variable()) {
            type* t = sym->as_variable().t;
            if (t->is_pointer(eptr_type::NAKED)) {
                t = t->as_pointer().t;
            }
            if (t->is_struct()) {
                sym = t->as_struct().ste->as_type().st->get(tok.value);
            } else if (t->is_union()) {
                sym = t->as_union().ste->as_type().st->get(tok.value);
            }
        }
    }
    
    if (!sym) {
        std::stringstream ss{};
        ss << '"' << tok.value << '"' << "does not exist";
        return error(ss.str(), epanic_mode::NO_PANIC, &tok);
    }
    return ast::symbol(sym, tok.value);
}

ast* parser::compileriden() {
    next(); // Skip
    return nullptr; // TODO
}

ast* parser::compileropts() {
    return nullptr; // TODO
}

ast* parser::compilernote() {
    if (is(TokenType::COMPILER_IDENTIFIER)) {
        return compileriden();
    } else if (is(Symbol::COMPILER)) {
        return compileropts();
    }
    return nullptr;
}

ast* parser::number() {
    compiler_assert(TokenType::NUMBER);
    return ast::qword(next().as_integer(), types.t_long); // TODO Separate
}

ast* parser::string() {
    compiler_assert(TokenType::STRING);
    std::string str = next().as_string(); // String
    u8* chars = new u8[str.length()];
    std::memcpy(chars, str.data(), str.length());
    return ast::string(chars, str.length());
}

ast* parser::character() {
    compiler_assert(TokenType::CHARACTER);
    u8 len = c.value.length();
    u32 ch = 0;
    std::memcpy(&ch, next().value.data(), len); // Char
    return ast::dword(ch, types.t_char);
}

ast* parser::array() {
    compiler_assert(Symbol::BRACKET_LEFT);
    std::vector<ast*> elems{};
    type* t = ctx().expected; // Needs to be set as the type of the array
    
    if (!t->is_pointer(eptr_type::ARRAY)) {
        throw parser_exception{};
    }
    
    t = t->as_pointer().t;
    
    push_context();
    ctx().expected = t; // aexpression() requires we set an expected value type
    auto cg = guard();
    
    do {
        next(); // [ ,
        if (is(Symbol::BRACKET_RIGHT)) {
            break;
        } else {
            ast* exp = aexpression();
            if (!exp->get_type()->can_weak_cast(t)) { // Eventually ANY arrays will be a thing, not for now
                exp = error("Cannot cast to array type", epanic_mode::IN_ARRAY);
            } else if (t == types.t_let) {
                t = exp->get_type();
            }
            elems.push_back(exp);
        }
    } while (is(Symbol::COMMA));
    require(Symbol::BRACKET_RIGHT, epanic_mode::ESCAPE_BRACKET);
    
    next(); // ]
    
    cg.deactivate();
    
    ast* ret = ast::array(nullptr, elems.size(), ctx().expected);
    std::memcpy(ret->as_array().elems, elems.data(), elems.size() * sizeof(ast*));
    return ret;
}

ast* parser::struct_lit() {
    compiler_assert(Symbol::BRACE_LEFT);
    type* t = ctx().expected; // Needs to be set as the type of the struct
    
    if (!t->is_struct(false)) {
        throw parser_exception{};
    }
    
    auto& s = t->as_struct();
    symbol_table* st = s.ste->as_type().st;
    ast* ret = ast::_struct(t);
    std::vector<ast*> elems{s.fields.size(), nullptr};
    
    for (u64 i = 0; i < s.fields.size(); ++i) {
        if (s.fields[i].value) {
            elems[i] = s.fields[i].value; // Default values go here wooo
        }
    }
    
    bool named = false;
    u64 field = 0;
    
    do {
        next(); // { ,
        if (is(Symbol::BRACE_RIGHT)) {
            break;
        }
        
        if (named || peek(Symbol::ASSIGN)) {
            named = true;
            std::string name = c.value;
            
            next(); // name
            require(Symbol::ASSIGN);
            
            st_entry* fld = st->get(name, false);
            if (!fld || !fld->is_field()) {
                error("Not a field", epanic_mode::COMMA);
                continue;
            }
            field = fld->as_field().field;
            
            push_context();
            ctx().expected = s.fields[field].t; // Don't do defensive programming here too...
            auto cg = guard();
            
            ast* exp = aexpression();
            if (!exp->get_type()->can_weak_cast(t)) {
                exp = error("Cannot cast to correct type", epanic_mode::IN_STRUCT_LIT);
            }
            elems[field] = exp;
            
        } else {
            push_context();
            ctx().expected = s.fields[field].t;
            auto cg = guard();
            
            ast* exp = aexpression();
            if (!exp->get_type()->can_weak_cast(t)) {
                exp = error("Cannot cast to correct type", epanic_mode::IN_STRUCT_LIT);
            }
            elems[field] = exp;
            ++field;
        }
        
    } while (is(Symbol::COMMA));
    require(Symbol::BRACE_RIGHT, epanic_mode::ESCAPE_BRACE);
    
    next(); // }
    
    std::memcpy(ret->as_struct().elems, elems.data(), elems.size() * sizeof(ast*));
    return ret;
}

ast* parser::safe_literal() {
    if (is(TokenType::CHARACTER)) {
        return character();
    } else if (is(TokenType::NUMBER)) {
        return number();
    } else if (is(TokenType::STRING)) {
        return string();
    } else if (is(Keyword::NNULL)) {
        next(); // null
        return ast::qword(0, types.t_null);
    } else if (is(Keyword::TRUE)) { 
        next(); // true
        return ast::byte(1, types.t_bool);
    } else if (is(Keyword::FALSE)) {
        next(); // false
        return ast::byte(0, types.t_bool);
    } else {
        next(); // ?????
        return error("Invalid literal");
    }
}

ast* parser::compound_literal() {
    if (is(Symbol::BRACE_LEFT)) { // struct
        if (!ctx().expected->is_struct(false)) {
            return error("Expected type was not a struct", epanic_mode::ESCAPE_BRACE);
        } else {
            return struct_lit();
        }
    } else if (is(Symbol::BRACKET_LEFT)) { // array
        if (!ctx().expected->is_pointer(eptr_type::ARRAY)) {
            return error("Expected type was not an array", epanic_mode::ESCAPE_BRACKET);
        } else {
            return array();
        }
    } else {
        next(); // ????
        return error("Invalid compound literal");
    }
}

ast* parser::program() {
    ast* prog = ast::block(st());
    auto& stmts = prog->as_block().stmts;
    
    while (true) {
        if (is(Keyword::USING)) {
            stmts.push_back(usingstmt());
        } else if (is(Keyword::IMPORT)) {
            stmts.push_back(importstmt());
        } else if (is(Keyword::NAMESPACE)) {
            stmts.push_back(namespacestmt());
        } else if (is_decl_start()){
            stmts.push_back(freedeclstmt());
        } else if (is(TokenType::END_OF_FILE)) {
            break;
        } else {
            stmts.push_back(error("Invalid token", epanic_mode::SEMICOLON));
        }
    }
    
    if (!errors.empty()) {
        for (auto& [t, e] : errors) {
            logger::error() << t.get_info() << " - " << e << "\n\n"; // TODO Clearer errors, of course
        }
    }
    
    return prog;
}

ast* parser::statement() {
    while(is_compiler_token()) {
        compilernote();
    }
    
    ast* ret{nullptr};
    
    if (is(Keyword::IF)) {
        ret = ifstmt();
    } else if (is(Keyword::FOR)) {
        ret = forstmt();
    } else if (is(Keyword::WHILE)) {
        ret = whilestmt();
    } else if (is(Keyword::SWITCH)) {
        ret = switchstmt();
    } else if (is(Keyword::TRY)){
        ret = trystmt();
    } else if (is(Keyword::RETURN)) {
        ret = returnstmt();
    } else if (is(Keyword::RAISE)) {
        ret = raisestmt();
    } else if (is(Keyword::GOTO)) {
        ret = gotostmt();
    } else if (is(Keyword::LABEL)) {
        ret = labelstmt();
    } else if (is(Keyword::DEFER)) {
        ret = deferstmt();
    } else if (is(Keyword::BREAK)) {
        ret = breakstmt();
    } else if (is(Keyword::CONTINUE)) {
        ret = continuestmt();
    } else if (is(Keyword::LEAVE)) {
        ret = leavestmt();
    } else if (is(Keyword::USING)) {
        ret = usingstmt();
    } else if (is(Keyword::NAMESPACE)) {
        ret = namespacestmt();
    } else if (is_decl_start()) {
        ret = freedeclstmt();
    } else if (is(Symbol::BRACE_LEFT)) {
        ret = scope();
    } else {
        ret = mexpression();
    }
    
    return ret;
}

ast* parser::scopestatement() {
    while(is_compiler_token()) {
        compilernote();
    }
    
    ast* ret{nullptr};
    
    if (is(Keyword::IF)) {
        ret = ifstmt();
    } else if (is(Keyword::FOR)) {
        ret = forstmt();
    } else if (is(Keyword::WHILE)) {
        ret = whilestmt();
    } else if (is(Keyword::SWITCH)) {
        ret = switchstmt();
    } else if (is(Keyword::RETURN)) {
        ret = returnstmt();
    } else if (is(Keyword::RAISE)) {
        ret = raisestmt();
    } else if (is(Keyword::GOTO)) {
        ret = gotostmt();
    } else if (is(Keyword::BREAK)) {
        ret = breakstmt();
    } else if (is(Keyword::CONTINUE)) {
        ret = continuestmt();
    } else if (is(Symbol::BRACE_LEFT)) {
        ret = scope();
    } else {
        ret = mexpression();
    }
    
    return ret;
}

ast* parser::scope() {
    compiler_assert(Symbol::BRACE_LEFT);
    
    next(); // {
    
    push_context();
    ctx().st = st()->make_child();
    auto cg = guard();
    
    ast* block = ast::block(st());
    
    while(!is(Symbol::BRACE_RIGHT) && !is(TokenType::END_OF_FILE)) {
        block->as_block().stmts.push_back(statement());
    }
    
    require(Symbol::BRACE_RIGHT, epanic_mode::ESCAPE_BRACE);
    next(); // }
    
    return block;
}

ast* parser::ifstmt() {
    compiler_assert(Keyword::IF);
    next(); // if
    
    push_context();
    ctx().st = st()->make_child();
    auto cg = guard();
    
    ast* conditions = ast::block(st());
    auto& stmts = conditions->as_block().stmts;
    
    eexpression_type last{eexpression_type::INVALID};
    do {
        stmts.push_back(fexpression(&last));
    } while (is(Symbol::SEMICOLON));
    
    if (last != eexpression_type::EXPRESSION) {
        error("Last part of if conditions must be an expression"); // TODO better message
    }
    
    if (!stmts.back()->get_type()->can_boolean()) {
        error("Last condition on an if must convert to boolean");
    }
    
    ast* results{nullptr};
    
    if (is(Symbol::BRACE_LEFT)) {
        results = ifscope();
    } else if (is(Keyword::DO)) {
        next(); // do
        results = scopestatement();
    } else {
        results = error("Expected do or brace after if conditions", epanic_mode::SEMICOLON);
    }
    
    return ast::binary(Symbol::KWIF, conditions, results);
}

ast* parser::ifscope() {
    compiler_assert(Symbol::BRACE_LEFT);
    ast* s = scope();
    
    if (is(Keyword::ELSE)) {
        next(); // else
        ast* es = scopestatement();
        return ast::binary(Symbol::KWELSE, s, es);
    } else {
        return s;
    }
}

ast* parser::forstmt() {
    compiler_assert(Keyword::FOR);
    next(); // for
    
    push_context();
    ctx().st = st()->make_child();
    auto cg = guard();
    
    ast* condition = forcond();
    
    ast* loop{nullptr};
    
    if (is(Symbol::BRACE_LEFT)) {
        loop = scope();
    } else if (is(Keyword::DO)) {
        next(); // do
        loop = scopestatement();
    } else {
        loop = error("Expected do or brace after for conditions", epanic_mode::SEMICOLON);
    }
    
    return ast::binary(Symbol::KWFOR, condition, loop);
}

ast* parser::forcond() {
    enum class fortype {
        INVALID, CLASSIC, FOREACH, LUA
    } ftype{fortype::INVALID};
    
    ast* start{nullptr};
    if (is(Symbol::SEMICOLON)) {
        ftype = fortype::CLASSIC;
    } else if (is_type() || is_infer()) {
        ast* new_iden = freevardecliden();
        if (is(Symbol::COLON)) {
            ftype = fortype::FOREACH;
            start = new_iden;
        } else {
            require(Symbol::ASSIGN);
            ast* idenass = vardeclass();
            if (is(Symbol::BRACE_LEFT) || is(Keyword::DO)) {
                ftype = fortype::LUA;
            } else {
                ftype = fortype::CLASSIC;
            }
            start = ast::binary(Symbol::ASSIGN, new_iden, idenass);
        }
    } else if (is(TokenType::IDENTIFIER)) {
        start = assignment();
        if (is(Symbol::BRACE_LEFT) || is(Keyword::DO)) {
            ftype = fortype::LUA;
        } else {
            ftype = fortype::CLASSIC;
        }
    } else {
        ftype = fortype::CLASSIC;
        start = fexpression();
    }
    
    switch (ftype) {
        case fortype::CLASSIC: { // for x; y; z { }
            compiler_assert(Symbol::SEMICOLON);
            next(); // ;
            
            ast* parts = ast::block(st());
            auto& stmts = parts->as_block().stmts;
            stmts.push_back(start);
            
            ast* condition{nullptr};
            if (is(Symbol::SEMICOLON)) {
                condition = ast::byte(1, types.t_bool); // true
            } else {
                condition = expression();
            }
            
            if (!condition->get_type()->can_boolean()) {
                condition = error("For condition must convert to boolean");
            }
            
            stmts.push_back(condition);
            if (is(Symbol::BRACE_LEFT) || is(Keyword::DO)) {
                stmts.push_back(nullptr);
            } else {
                do {
                    stmts.push_back(mexpression());
                } while (!is(Symbol::BRACE_LEFT) && !is(Keyword::DO) && !is(TokenType::END_OF_FILE));
            }
            
            return ast::unary(Symbol::KWFORCLASSIC, parts);
        }
        case fortype::FOREACH: { // for x : y { }
            compiler_assert(Symbol::COLON);
            ast* colon = expression();
            if (!colon->get_type()->is_pointer(eptr_type::ARRAY)) {
                colon = error("For each value must be an array");
            } else if (!colon->get_type()->as_pointer().t->can_weak_cast(start->get_type())) {
                colon = error("Cannot convert between types in for each");
            }
            return ast::unary(Symbol::KWFOREACH, ast::binary(Symbol::COLON, start, colon));
        }
        case fortype::LUA: { // for x = y, z, w { }
            auto& luablock = start->as_binary();
            auto& commas   = luablock.right->as_block();
            auto len = commas.stmts.size();
            if (len < 2 || len > 3) {
                error("Lua for had an illegal number of values");
            }
            return ast::unary(Symbol::KWFORLUA, start);
        }
        case fortype::INVALID: [[fallthrough]];
        default:
            return error("Invalid for type", epanic_mode::ULTRA_PANIC);
    }
}

ast* parser::whilestmt() {
    compiler_assert(Keyword::WHILE);
    next(); // while
    
    push_context();
    ctx().st = st()->make_child();
    auto cg = guard();
    
    ast* conditions = ast::block(st());
    auto& stmts = conditions->as_block().stmts;
    
    eexpression_type last{eexpression_type::INVALID};
    do {
        stmts.push_back(fexpression(&last));
    } while (is(Symbol::SEMICOLON));
    
    if (last != eexpression_type::EXPRESSION) {
        error("Last part of while conditions must be an expression"); // TODO better message
    }
    
    if (!stmts.back()->get_type()->can_boolean()) {
        error("Last condition on an while must convert to boolean");
    }
    
    ast* results{nullptr};
    
    if (is(Symbol::BRACE_LEFT)) {
        results = scope();
    } else if (is(Keyword::DO)) {
        next(); // do
        results = scopestatement();
    } else {
        results = error("Expected do or brace after while conditions", epanic_mode::SEMICOLON);
    }
    
    return ast::binary(Symbol::KWWHILE, conditions, results);
}

ast* parser::switchstmt() {
    compiler_assert(Keyword::SWITCH);
    next(); // switch
    
    push_context();
    ctx().st = st()->make_child();
    auto cg = guard();
    
    ast* conditions = ast::block(st());
    auto& stmts = conditions->as_block().stmts;
    
    eexpression_type last{eexpression_type::INVALID};
    do {
        stmts.push_back(fexpression(&last));
    } while (is(Symbol::SEMICOLON));
    
    if (last != eexpression_type::EXPRESSION) {
        error("Last part of switch conditions must be an expression"); // TODO better message
    }
    
    type* switcht = stmts.back()->get_type();
    if (switcht->is_combination()) {
        error("Cannot switch on a combination type"); // TODO Panic, please
    }
    ctx().aux = switcht;
    
    ast* cases{nullptr};
    
    if (!is(Symbol::BRACE_LEFT)) {
        cases = error("Expected brace to start cases", epanic_mode::ESCAPE_BRACE);
    } else {
        cases = switchscope();
    }
    
    return ast::binary(Symbol::KWSWITCH, conditions, cases);
}

ast* parser::switchscope() {
    compiler_assert(Symbol::BRACE_LEFT);
    next(); // {
    
    ast* cases = ast::block(st());
    auto& stmts = cases->as_block().stmts;
    
    while (!is(Symbol::BRACE_RIGHT) && !is(TokenType::END_OF_FILE)) {
        stmts.push_back(casestmt());
    }
    
    if (require(Symbol::BRACE_RIGHT, epanic_mode::ESCAPE_BRACE)) {
        next(); // }
    }
    
    return cases;
}

ast* parser::casestmt() {
    ast* caseast = ast::binary();
    auto& bin = caseast->as_binary();
    
    if (is(Keyword::ELSE)) {
        bin.op = Symbol::KWELSE;
        next(); // else
    } else if (is(Keyword::CASE)) {
        bin.op = Symbol::KWCASE;
        next(); // case
        ast* case_vals = ast::block(st());
        auto& stmts = case_vals->as_block().stmts;
        
        do {
            ast* exp = expression();
            if (!exp->get_type()->can_weak_cast(ctx().aux)) {
                exp = error("Cannot cast case value to switch value");
            }
            stmts.push_back(exp);
            if (!is(Symbol::COMMA)) {
                break;
            } else {
                next(); // ,
            }
        } while (!is(Symbol::BRACE_LEFT) && !is(Keyword::CONTINUE) && 
                 !is(Keyword::DO) && !is(TokenType::END_OF_FILE));
        
        bin.left = case_vals;
        
    } else {
        return error("Expected either case or else", epanic_mode::ESCAPE_BRACE);
    }
    
    if (is(Keyword::CONTINUE)) {
        bin.right = ast::unary(Symbol::KWCONTINUE);
    } else if (is(Symbol::BRACE_LEFT)) {
        bin.right = ast::unary(Symbol::KWCASE, scope());
    } else if (is(Keyword::DO)) {
        next(); // do
        bin.right = ast::unary(Symbol::KWCASE, scopestatement());
    } else {
        bin.right = error("Expected continue, do or a scope", epanic_mode::ESCAPE_BRACE);
    }
    
    return caseast;
}

ast* parser::trystmt() {
    compiler_assert(Keyword::TRY);
    next(); // try
    
    if (require(Symbol::COLON)) {
        next(); // :
    }
    
    ast* tryblock = ast::block(st());
    auto& stmts = tryblock->as_block().stmts;
    
    do {
        stmts.push_back(statement());
    } while (!is(Keyword::CATCH) && !is(TokenType::END_OF_FILE));
    
    ast* consequence = ast::unary();
    auto& un = consequence->as_unary();
    
    if (is(Symbol::PAREN_LEFT)) {
        
        push_context();
        ctx().st = st()->make_child();
        auto cg = guard();
        
        next(); // (
        un.op = Symbol::KWCATCH;
        if (is(Keyword::SIG)) {
            next(); // sig
        }
        
        if (require(TokenType::IDENTIFIER, epanic_mode::ESCAPE_PAREN)) {
            st()->add_variable(c.value, types.t_sig);
            next(); // iden
        }
        
        require(Symbol::PAREN_RIGHT, epanic_mode::ESCAPE_BRACE);
        next(); // (
        
        require(Symbol::BRACE_LEFT, epanic_mode::ESCAPE_BRACE);
        
        ctx().aux = types.t_sig;
        un.node = switchscope();
    } else if (is(Keyword::RAISE)) {
        next(); // raise
        un.op = Symbol::KWRAISE;
        require(Symbol::SEMICOLON, epanic_mode::SEMICOLON);
        next(); // ;
    } else {
        un.op = Symbol::SYMBOL_INVALID;
        un.node = error("Expected parenthesis or raise after catch");
    }
    
    return ast::binary(Symbol::KWTRY, tryblock, consequence);
}

ast* parser::returnstmt() {
    compiler_assert(Keyword::RETURN);
    
    next(); // return
    
    type*& ret_type = ctx().function->as_function().rets;
    ast* ret = ast::unary(Symbol::KWRETURN);
    auto& un = ret->as_unary();
    
    if (is(Keyword::VOID) || is(Symbol::SEMICOLON)) {
        if (is(Keyword::VOID)) {
            next(); // void
        }
        require(Symbol::SEMICOLON, epanic_mode::SEMICOLON);
        next(); // ;
        un.node = ast::none();
        un.t = types.t_void;
    } else {
        ast* rets = ast::block(st());
        auto& stmts = rets->as_block().stmts;
        type rtype = type{ettype::COMBINATION};
        
        do {
            if (is(Symbol::SEMICOLON)) {
                break;
            }
            ast* exp = aexpression();
            rtype.as_combination().types.push_back(exp->get_type());
            stmts.push_back(exp);
        } while (is(Symbol::COMMA));
        require(Symbol::SEMICOLON, epanic_mode::SEMICOLON);
        next(); // ;
        
        un.node = rets;
        un.t = types.get_or_add(rtype);
    }
    
    if (ret_type == types.t_let) {
        ret_type = un.t;
    } else {
        if (ret_type->is_combination()) {
            auto& rcomb = ret_type->as_combination();
            if (un.t->is_combination()) {
                auto& uncomb = un.t->as_combination();
                int i = 0, j = 0;
                while (i < rcomb.types.size() && j < uncomb.types.size()) {
                    while (i < rcomb.types.size() && rcomb.types[i] == types.t_sig && !uncomb.types[j]->can_weak_cast(types.t_sig)) {
                        ++i;
                    }
                    if (i >= rcomb.types.size()) {
                        break;
                    }
                    if (!uncomb.types[j]->can_weak_cast(rcomb.types[i])) {
                        un.node = error("Cannot cast return type to function return type");
                        break;
                    }
                    
                    ++i;
                    ++j;
                }
                
                while (i < rcomb.types.size()) {
                    if (rcomb.types[i] != types.t_sig) {
                        un.node = error("Incomplete return");
                        break;
                    }
                    ++i;
                }
                
                if (i != rcomb.types.size() || j != uncomb.types.size()) {
                    un.node = error("Incomplete return"); // TODO Better message
                }
                
            } else {
                int i = 0;
                while (i < rcomb.types.size() && rcomb.types[i] == types.t_sig) {
                    ++i;
                }
                if (i >= rcomb.types.size()) {
                    un.node = error("Return type not found");
                } else if (!un.t->can_weak_cast(rcomb.types[i])) {
                    un.node = error("Cannot cast return type to function return type");
                }
                ++i;
                while (i < rcomb.types.size() && rcomb.types[i] == types.t_sig) {
                    ++i;
                }
                if (i != rcomb.types.size()) {
                    un.node = error("Only one return given when more were needed");
                }
            }
        } else {
            if (!un.t->can_weak_cast(ret_type)) {
                un.node = error("Cannot cast return type to function return type");
            }
        }
    }
    
    return ret;
}

ast* parser::raisestmt() {
    compiler_assert(Keyword::RAISE);
    next(); // raise
    
    ast* ret{nullptr};
    
    if (is(TokenType::IDENTIFIER)) {
        auto entry = st()->get(c.value);
        if (!entry) { // Assume it's a SIG name
            symbol_table* overload_st = ctx().function->as_function().ste->as_function().st;
            auto sigval = overload_st->get(c.value);
            u64 val{0};
            if (!sigval) {
                val = overload_st->get_size(false);
                overload_st->add_field(c.value, val);
            } else {
                val = sigval->as_field().field;
            }
            ret = ast::unary(Symbol::KWRAISE, ast::qword(val, types.t_sig));
        } else { // Assume it's an expression
            ast* exp = expression();
            if (!exp->get_type()->can_weak_cast(types.t_sig)) {
                exp = error("Expression is not of sig type");
            }
            ret = ast::unary(Symbol::KWRAISE, exp);
        }
    } else { // Assume it's an expression
        ast* exp = expression();
        if (!exp->get_type()->can_weak_cast(types.t_sig)) {
            exp = error("Expression is not of sig type");
        }
        ret = ast::unary(Symbol::KWRAISE, exp);
    }
    
    require(Symbol::SEMICOLON, epanic_mode::SEMICOLON);
    next(); // ;
    
    type*& ret_type = ctx().function->as_function().rets;
    
    if (ret_type == types.t_void) {
        ret_type = types.t_sig;
    } else if (ret_type == types.t_let) { // TODO Allow this
        error("Cannot raise in incompletely typed non-inferred function return");
    } else if (!ret_type->is_combination() && ret_type != types.t_sig) {
        type nret = type(ettype::COMBINATION);
        nret.as_combination().types.push_back(ret_type);
        nret.as_combination().types.push_back(types.t_sig);
        ret_type = types.get_or_add(nret);
    } else if (ret_type->is_combination()) {
        bool has_sig = false;
        for (auto& typ : ret_type->as_combination().types) {
            if (typ == types.t_sig) {
                has_sig = true;
                break;
            }
        }
        if (!has_sig) {
            ret_type->as_combination().types.push_back(types.t_sig);
        }
    }
    
    return ret;
}

ast* parser::gotostmt() {
    compiler_assert(Keyword::GOTO);
    next(); // goto
    
    // TODO 
    
    next(); // iden
    require(Symbol::SEMICOLON, epanic_mode::SEMICOLON);
    next(); // ;
    return error("Not yet implemented");
}

ast* parser::labelstmt() {
    compiler_assert(Keyword::LABEL);
    next(); // goto
    
    // TODO 
    
    next(); // iden
    require(Symbol::SEMICOLON, epanic_mode::SEMICOLON);
    next(); // ;
    return error("Not yet implemented");
}

ast* parser::deferstmt() {
    compiler_assert(Keyword::DEFER);
    next(); // defer
    
    ast* stmt = statement();
    require(Symbol::SEMICOLON, epanic_mode::SEMICOLON);
    next(); // ;
    return ast::unary(Symbol::KWDEFER, stmt);
}

ast* parser::breakstmt() {
    compiler_assert(Keyword::BREAK);
    next(); // break
    
    require(Symbol::SEMICOLON, epanic_mode::SEMICOLON);
    next(); // ;
    return ast::unary(Symbol::KWBREAK);
}

ast* parser::continuestmt() {
    compiler_assert(Keyword::CONTINUE);
    next(); // continue
    
    require(Symbol::SEMICOLON, epanic_mode::SEMICOLON);
    next(); // ;
    return ast::unary(Symbol::KWCONTINUE);
}

ast* parser::leavestmt() {
    compiler_assert(Keyword::LEAVE);
    next(); // leave
    
    require(Symbol::SEMICOLON, epanic_mode::SEMICOLON);
    next(); // ;
    return ast::unary(Symbol::KWLEAVE);
}

ast* parser::importstmt() {
    compiler_assert(Keyword::IMPORT);
    next(); // import
    
    ast* what{nullptr};
    std::string as{};
    
    if (is(TokenType::STRING)) {
        what = string();
    } else {
        std::stringstream path{};
        path << c.value;
        as = c.value;
        next(); // iden
        while (is(Symbol::DOT)) {
            path << c.value;
            next(); // .
            path << c.value; 
            as = c.value;
            next(); // iden
        }
        std::string pathstr{path.str()};
        what = ast::string(pathstr);
    }
    
    ast* ret = ast::binary(Symbol::KWIMPORT, what);
    
    if (is(Keyword::AS)) {
        next(); // as
        if (st()->get(c.value)) {
            error("Identifier already exists");
        } else {
            as = c.value;
        }
        next(); // iden
    }
    
    ret->as_binary().right = ast::string(as);
    
    require(Symbol::SEMICOLON, epanic_mode::SEMICOLON);
    next(); // ;
    
    // TODO Do import
    return ret;
}

ast* parser::usingstmt() {
    compiler_assert(Keyword::USING);
    
    std::vector<std::string> path{};
    std::string as{};
    
    do {
        next(); // using, .
        require(TokenType::IDENTIFIER);
        
        path.push_back(c.value);
        next(); //iden
        
    } while(is(Symbol::DOT));
    
    if (is(Keyword::AS)) {
        next(); // as
        require(TokenType::IDENTIFIER);
        as = c.value;
        next();
    } else {
        as = path.back();
    }
    
    require(Symbol::SEMICOLON, epanic_mode::SEMICOLON);
    
    symbol_table* cur{st()};
    bool first = true;
    bool last = false;
    bool merge = false;
    st_entry* entry{nullptr};
    
    for (auto& str : path) {
        if (last) {
            return error("Path ends prematurely");
        }
        entry = cur->get(str, first);
        first = false;
        if (!entry) {
            return error("Given identifier does not exist");
        } else if (entry->is_type()) {
            merge = false;
            if (!entry->as_type().st) {
                last = true;
            } else {
                cur = entry->as_type().st;
            }
        } else if (entry->is_function()) {
            merge = false;
            cur = entry->as_function().st;
        } else if (entry->is_namespace()) {
            cur = entry->as_namespace().st;
            merge = true;
        } else {
            return error("Cannot use element found");
        }
    }
    
    if (merge) {
        st()->merge_st(cur);
    } else {
        st()->borrow(as, entry);
    }
    
    return ast::unary(Symbol::KWUSING);
}

ast* parser::namespacestmt() {
    compiler_assert(Keyword::NAMESPACE);
    
    std::vector<std::string> path{};
    
    do {
        next(); // namespace, .
        require(TokenType::IDENTIFIER);
        
        path.push_back(c.value);
        next(); //iden
        
    } while(is(Symbol::DOT));
    
    require(Symbol::BRACE_LEFT);
    
    symbol_table* cur{st()};
    st_entry* entry{nullptr};
    bool first = true;
    bool create = false;
    
    for (auto& str : path) {
        if (create) {
            return error("Cannot create nested namespaces", epanic_mode::ESCAPE_BRACE);
        }
        entry = cur->get(str, first);
        first = false;
        if (!entry) {
            create = true;
        } else if (!entry->is_namespace()) {
            return error("Namespaces can only be nested inside other namespaces", epanic_mode::ESCAPE_BRACE);
        } else {
            cur = entry->as_namespace().st;
        }
    }
    
    symbol_table* nsst{cur};
    
    if (create) {
        nsst = cur->make_child(etable_owner::NAMESPACE);
        cur->add_namespace(path.back(), nsst);
    }
    
    push_context();
    ctx().st = nsst;
    auto cg = guard();
    
    ast* nsscope{nullptr};
    if(is(Symbol::BRACE_LEFT)) {
         nsscope = namespacescope();
    } else {
        nsscope = error("Require { to start namespace", epanic_mode::ESCAPE_BRACE);
    }
    
    return ast::unary(Symbol::KWNAMESPACE, nsscope);
}

ast* parser::namespacescope() {
    compiler_assert(Symbol::BRACE_LEFT);
    next(); // {
    
    ast* ns = ast::block(st());
    auto& stmts = ns->as_block().stmts;
    
    while (true) {
        if (is(Keyword::USING)) {
            stmts.push_back(usingstmt());
        } else if (is(Keyword::NAMESPACE)) {
            stmts.push_back(namespacestmt());
        } else if (is_decl_start()){
            stmts.push_back(freedeclstmt());
        } else if (is(TokenType::END_OF_FILE)) {
            break;
        } else {
            stmts.push_back(error("Invalid token", epanic_mode::SEMICOLON));
        }
    }
    
    require(Symbol::BRACE_LEFT, epanic_mode::ESCAPE_BRACE);
    
    return ns;
}

type_flags parser::typemod() {
    type_flags ret{0};
    
    while (true) {
        switch(c.as_keyword()) {
            case Keyword::SIGNED:
                ret |= etype_flags::SIGNED;
                break;
            case Keyword::UNSIGNED:
                ret &= ~etype_flags::SIGNED;
                break;
            case Keyword::CONST:
                ret |= etype_flags::CONST;
                break;
            case Keyword::VOLATILE:
                ret |= etype_flags::VOLATILE;
                break;
            default:
                goto out_while;
        }
        next();
    }
    
    out_while:
    return ret;
}

ast* parser::arraysize() {
    compiler_assert(Symbol::BRACKET_LEFT);
    next(); // [
    
    ast* ret{nullptr};
    
    if (!is(Symbol::BRACKET_RIGHT)) {
        ret = expression();
    } else {
        ret = ast::none();
    }
    
    require(Symbol::BRACKET_RIGHT);
    next(); // ]
    
    return ret;
}

ast* parser::nntype() {
    ast* ret = ast::nntype();
    auto& nnt = ret->as_nntype();
    type* t{nullptr};
    
    if (is(TokenType::KEYWORD)) {
        switch (c.as_keyword()) {
            case Keyword::BYTE:
                t = types.t_byte;
                break;
            case Keyword::SHORT:
                t = types.t_short;
                break;
            case Keyword::INT:
                t = types.t_int;
                break;
            case Keyword::LONG:
                t = types.t_long;
                break;
            case Keyword::SIG:
                t = types.t_sig;
                break;
            case Keyword::FLOAT:
                t = types.t_float;
                break;
            case Keyword::DOUBLE:
                t = types.t_double;
                break;
            case Keyword::BOOL:
                t = types.t_bool;
                break;
            case Keyword::CHAR:
                t = types.t_char;
                break;
            case Keyword::STRING:
                t = types.t_string;
                break;
            case Keyword::FUN: {
                ast* ft = functype();
                nnt.t = ft->as_nntype().t;
                break;
            }
            default:
                nnt.t = types.t_void;
                error("Invalid type", epanic_mode::SEMICOLON);
                return ret;
        }
        next();
    } else if(is(TokenType::IDENTIFIER)) {
        st_entry* type_entry = st()->get(c.value); // TODO Dot operator?
        if (!type_entry || !type_entry->is_type()) {
            nnt.t = types.t_void;
            error("Invalid type", epanic_mode::SEMICOLON);
            return ret;
        } else {
            t = type_entry->as_type().t;
        }
        next();
    } else {
        nnt.t = types.t_void;
        error("Invalid type", epanic_mode::SEMICOLON);
        return ret;
    }
    
    type constructed = *t;
    constructed.id = 0;
    constructed.flags = typemod();
    
    t = types.get_or_add(constructed);
    
    while (is_pointer_type() || is(Symbol::BRACKET_LEFT)) {
        constructed = type{ettype::POINTER};
        constructed.as_pointer().t = t;
        
        if (is(Symbol::BRACKET_LEFT)) {
            constructed.as_pointer().ptr_t = eptr_type::ARRAY;
            ast* size = arraysize();
            if (!size->get_type()->can_weak_cast(types.t_long)) {
                size = error("Array size must be a long type");
            } 
            // TODO Const expression?
            nnt.array_sizes.push_back(size);
        } else {
            switch (c.as_symbol()) {
                case Symbol::POINTER:
                    constructed.as_pointer().ptr_t = eptr_type::NAKED;
                    break;
                case Symbol::UNIQUE_POINTER:
                    constructed.as_pointer().ptr_t = eptr_type::UNIQUE;
                    break;
                case Symbol::SHARED_POINTER:
                    constructed.as_pointer().ptr_t = eptr_type::SHARED;
                    break;
                case Symbol::WEAK_POINTER:
                    constructed.as_pointer().ptr_t = eptr_type::WEAK;
                    break;
                default:
                    error("What the fuck just happened", epanic_mode::ULTRA_PANIC);
            }
        }
        
        constructed.flags = typemod();
        t = types.get_or_add(constructed);
    }
    
    nnt.t = t;
    
    return ret;
}

ast* parser::functype() {
    compiler_assert(Keyword::FUN);
    next(); // fun
    
    ast* ret = ast::nntype();
    auto& nnt = ret->as_nntype();
    type ftype = type{ettype::PFUNCTION};
    type frtype = type{ettype::COMBINATION};
    
    
    require(Symbol::LT);
    next(); // <
    
    while (!is(Symbol::PAREN_LEFT) && !is(TokenType::END_OF_FILE)) {
        ast* t = nntype();
        if (t->as_nntype().array_sizes.size()) {
            error("Function type arrays cannot have expression sizes");
        }
        frtype.as_combination().types.push_back(t->as_nntype().t);
        
        if (is(Symbol::COLON)) {
            next(); // :
        } else {
            require(Symbol::PAREN_LEFT);
        }
    }
    
    type* r = types.get_or_add(frtype);
    ftype.as_pfunction().rets = r;
    auto& params = ftype.as_pfunction().params;
    
    require(Symbol::PAREN_LEFT);
    
    while (!is(Symbol::PAREN_RIGHT) && !is(TokenType::END_OF_FILE)) {
        ast* t = nntype();
        if (t->as_nntype().array_sizes.size()) {
            error("Function type arrays cannot have expression sizes");
        }
        type* pt = t->as_nntype().t;
        param_flags pf{0};
        if (is(Symbol::SPREAD)) {
            next(); // ...
            pf |= eparam_flags::SPREAD;
        }
        if (is(Symbol::ASSIGN)) {
            next(); // =
            pf |= eparam_flags::DEFAULTABLE;
        }
        
        if (is(Symbol::COMMA)) {
            next();
        } else {
            require(Symbol::PAREN_RIGHT);
        }
        
        params.push_back({pt, pf});
    }
    
    require(Symbol::PAREN_RIGHT);
    next(); // )
    
    require(Symbol::GT);
    next(); // >
    
    type* rrt = types.get_or_add(ftype);
    nnt.t = rrt;
    return ret;
}

ast* parser::infer() {
    ast* ret = ast::nntype();
    type*& t = ret->as_nntype().t;
    
    switch (c.as_keyword()) {
        case Keyword::LET:
            t = types.t_let;
            break;
        default:
            t = types.t_void;
            error("No inference keyword found");
            break;
    }
    
    return ret;
}

ast* parser::freedeclstmt() {
    if (is(Keyword::STRUCT)) {
        return structdecl();
    } else if (is(Keyword::UNION)) {
        return uniondecl();
    } else if (is(Keyword::ENUM)) {
        return enumdecl();
    } else {
        if (is(Keyword::VOID)) {
            return funcdecl();
        }
        ast* t1 = nntype();
        if (is(Symbol::COLON)) {
            return funcdecl(t1);
        } else if (peek(Symbol::PAREN_LEFT)) {
            return funcdecl(t1);
        }
        ast* ret = freevardecl(t1);
        require(Symbol::SEMICOLON, epanic_mode::SEMICOLON);
        next(); // ;
        return ret;
    }
}

ast* parser::structdeclstmt() {
    if (is(Keyword::STRUCT)) {
        return structdecl();
    } else if (is(Keyword::UNION)) {
        return uniondecl();
    } else if (is(Keyword::ENUM)) {
        return enumdecl();
    } else {
        if (is(Keyword::VOID)) {
            return funcdecl();
        }
        ast* t1 = nntype();
        if (is(Symbol::COLON)) {
            return funcdecl(t1);
        } else if (peek(Symbol::PAREN_LEFT)) {
            return funcdecl(t1);
        }
        ast* ret = structvardecl(t1);
        require(Symbol::SEMICOLON, epanic_mode::SEMICOLON);
        next(); // ;
        return ret;
    }
}

ast* parser::uniondeclstmt() {
    if (is(Keyword::STRUCT)) {
        return structdecl();
    } else if (is(Keyword::UNION)) {
        return uniondecl();
    } else if (is(Keyword::ENUM)) {
        return enumdecl();
    } else {
        if (is(Keyword::VOID)) {
            return funcdecl();
        }
        ast* t1 = nntype();
        if (is(Symbol::COLON)) {
            return funcdecl(t1);
        } else if (peek(Symbol::PAREN_LEFT)) {
            return funcdecl(t1);
        }
        ast* ret = simplevardecl(t1);
        require(Symbol::SEMICOLON, epanic_mode::SEMICOLON);
        next(); // ;
        return ret;
    }
}

ast* parser::vardeclass() {
    compiler_assert(Symbol::ASSIGN);
    next(); // =
    
    ast* vals = ast::block(st());
    auto& stmts = vals->as_block().stmts;
    
    type* e = ctx().expected;
    
    if (e->is_combination()) {
        auto& types = e->as_combination().types;
        
        push_context();
        auto cg = guard();
        
        for (type* typ : types) {
            ctx().expected = typ;
            ast* exp = aexpression();
            if (!exp->get_type()->can_weak_cast(typ)) {
                error("Cannot cast expression to correct type");
            }
            stmts.push_back(exp);
            
            if (is(Symbol::SEMICOLON)) {
                break;
            } else {
                require(Symbol::COMMA, epanic_mode::COMMA);
                next(); // ,
            }
        }
        
    } else {
        ast* exp = aexpression();
        if (!exp->get_type()->can_weak_cast(e)) {
            error("Cannot cast expression to correct type");
        }
        stmts.push_back(exp);
    }
    
    require(Symbol::SEMICOLON, epanic_mode::SEMICOLON);
    next(); // ;
    
    return vals;
}

ast* parser::freevardecliden(ast* t1) {
    ast* ret = ast::block(st());
    auto& stmts = ret->as_block().stmts;
    
    ast* typ{nullptr};
    if (t1) {
        typ = t1;
    } else if (is_infer()) {
        typ = infer();
    } else {
        typ = nntype();
    }
    
    require(TokenType::IDENTIFIER);
    std::string name = c.value;
    next(); // iden
    
    type* expected = typ->as_nntype().t;
    delete typ;
    typ = nullptr;
    st_entry* entry = st()->add_variable(name, expected);
    
    stmts.push_back(ast::symbol(entry, name));
    
    if (is(Symbol::COMMA)) {
        type comb = type{ettype::COMBINATION};
        auto& types = comb.as_combination().types;
        types.push_back(expected);
    
        while (is(Symbol::COMMA)) {
            next(); // ,
            
            if (is_infer()) {
                typ = infer();
            } else if (is_type()) {
                typ = nntype();
            }
            
            require(TokenType::IDENTIFIER);
            name = c.value;
            next(); // iden
            
            expected = typ->as_nntype().t;
            delete typ;
            typ = nullptr;
            entry = st()->add_variable(name, expected);
            stmts.push_back(ast::symbol(entry, name));
            types.push_back(expected);
        }
        
        expected = parser::types.get_or_add(comb);
    }
    
    return ast::unary(Symbol::SYMDECL, ret, expected);
}

ast* parser::freevardecl(ast* t1) {
    ast* iden = freevardecliden(t1);
    
    if (is(Symbol::ASSIGN)) {
        push_context();
        ctx().expected = iden->as_unary().t;
        auto cg = guard();
        
        auto& stmts = iden->as_block().stmts;
        
        ast* values = vardeclass();
        auto& vals = values->as_block().stmts;
        
        for (u64 i = 0; i < stmts.size(); ++i) {
            if (i < vals.size()) {
                auto& var = stmts[i]->as_symbol().symbol->as_variable();
                var.value = vals[i];
                if (var.t == types.t_let) {
                    var.t = vals[i]->get_type();
                }
                var.defined = true;
                vals[i] = nullptr; // Ownership rescinded
            } else {
                if (stmts[i]->as_symbol().symbol->as_variable().t == types.t_let) {
                    error("Variable has uninferrable type");
                }
            }
        }
        
        delete values; // No longer needed
    }
    
    return iden;
}

ast* parser::structvardecliden(ast* t1) {
    auto& fields = ctx()._struct->as_struct().fields;
    
    ast* ret = ast::block(st());
    auto& stmts = ret->as_block().stmts;
    
    ast* typ{nullptr};
    if (t1) {
        typ = t1;
    } else if (is_infer()) {
        typ = infer();
    } else {
        typ = nntype();
    }
    
    require(TokenType::IDENTIFIER);
    std::string name = c.value;
    next(); // iden
    
    type* expected = typ->as_nntype().t;
    delete typ;
    typ = nullptr;
    st_entry* entry = st()->add_field(name, fields.size());
    
    field f;
    f.t = expected;
    f.name = name;
    
    if (is(Symbol::COLON)) {
        next(); // :
        
        require(TokenType::NUMBER);
        if (c.as_real() != c.as_integer()) {
            error("Bitfield values must be integers", epanic_mode::SEMICOLON);
        } else {
            u64 val = c.as_integer();
            if (val > 64 || val == 0) {
                error("Bitfield values must be between 0 and 64");
            }
            f.bits = (u8) val;
            f.bitfield = true;
        }
    }
    
    fields.push_back(f);
    
    stmts.push_back(ast::symbol(entry, name));
    
    if (is(Symbol::COMMA)) {
        type comb = type{ettype::COMBINATION};
        auto& types = comb.as_combination().types;
        types.push_back(expected);
    
        while (is(Symbol::COMMA)) {
            next(); // ,
            
            if (is_infer()) {
                typ = infer();
            } else if (is_type()) {
                typ = nntype();
            }
            
            require(TokenType::IDENTIFIER);
            name = c.value;
            next(); // iden
            
            expected = typ->as_nntype().t;
            delete typ;
            typ = nullptr;
            entry = st()->add_field(name, fields.size());
            
            field f;
            f.t = expected;
            f.name = name;
            
            if (is(Symbol::COLON)) {
                next(); // :
                
                require(TokenType::NUMBER);
                if (c.as_real() != c.as_integer()) {
                    error("Bitfield values must be integers", epanic_mode::SEMICOLON);
                } else {
                    u64 val = c.as_integer();
                    if (val > 64 || val == 0) {
                        error("Bitfield values must be between 0 and 64");
                    }
                    f.bits = (u8) val;
                    f.bitfield = true;
                }
            }
            
            fields.push_back(f);
            
            stmts.push_back(ast::symbol(entry, name));
            types.push_back(expected);
        }
        
        expected = parser::types.get_or_add(comb);
    }
    
    return ast::unary(Symbol::SYMDECL, ret, expected);
}

ast* parser::structvardecl(ast* t1) {
    ast* iden = structvardecliden(t1);
    
    if (is(Symbol::ASSIGN)) {
        push_context();
        ctx().expected = iden->as_unary().t;
        auto cg = guard();
        
        auto& stmts = iden->as_block().stmts;
        
        ast* values = vardeclass();
        auto& vals = values->as_block().stmts;
        
        auto& fields = ctx()._struct->as_struct().fields;
        
        for (u64 i = 0; i < stmts.size(); ++i) {
            if (i < vals.size()) {
                auto& var = stmts[i]->as_symbol().symbol->as_field();
                fields[var.field].value = vals[i];
                if (fields[var.field].t == types.t_let) {
                    fields[var.field].t = vals[i]->get_type();
                }
                vals[i] = nullptr; // Ownership rescinded
            } else {
                auto& var = stmts[i]->as_symbol().symbol->as_field();
                if (fields[var.field].t == types.t_let) {
                    error("Variable has uninferrable type");
                }
            }
        }
        
        delete values; // No longer needed
    }
    
    return iden;
}

ast* parser::simplevardecl(ast* t1) {
    auto& _union = ctx()._struct->as_union();
    auto& fields = _union.fields;
    
    ast* typ{nullptr};
    if (t1) {
        typ = t1;
    } else if (is_infer()) {
        typ = infer();
    } else {
        typ = nntype();
    }
    
    require(TokenType::IDENTIFIER);
    std::string name = c.value;
    next(); // iden
    
    type* expected = typ->as_nntype().t;
    delete typ;
    st_entry* entry = st()->add_field(name, fields.size());
    ast* ret = ast::symbol(entry, name);
    
    ufield f;
    f.t = expected;
    f.name = name;
    
    fields.push_back(f);
    
    if (is(Symbol::ASSIGN)) {
        next(); // =
        
        push_context();
        ctx().expected = expected;
        auto cg = guard();
        
        ast* exp = aexpression();
        type* exptype = exp->get_type();
        if (!exptype->can_weak_cast(expected)) {
            error("Cannot cast expression to correct type");
        }
        if (fields.back().t == types.t_let) {
            fields.back().t = exptype;
        }
        if (_union.def_value) {
            error("Unions can only have one default value");
        } else {
            _union.def_value = exp;
            _union.def_type = fields.size() - 1;
        }
    }
    
    return ast::unary(Symbol::SYMDECL, ret, expected);
}

type* parser::funcreturns(ast* t1) {
    ast* typ{nullptr};
    type* rettype{nullptr};
    if (is_infer()) {
        typ = infer();
        rettype = typ->as_nntype().t;
    } else if (is(Keyword::VOID)) {
        next(); // void
        rettype = types.t_void;
    } else {
        if (t1) {
            typ = t1;
        } else {
            typ = nntype(); // TODO Check array sizes, can't be expressions
        }
        
        rettype = typ->as_nntype().t;
        delete typ;
        typ = nullptr;
        
        if (is(Symbol::COLON)) {
            type comb = type{ettype::COMBINATION};
            auto& ctypes = comb.as_combination().types;
            ctypes.push_back(rettype);
            
            while (is(Symbol::COLON)) {
                next();
                typ = nntype();
                ctypes.push_back(typ->as_nntype().t);
                delete typ;
                typ = nullptr;
            }
            
            rettype = types.get_or_add(comb);
        }
    }
    
    return rettype;
}

ast* parser::funcdecl(ast* t1, type* thistype) {
    type functype = type{ettype::FUNCTION};
    auto& func = functype.as_function();
    func.rets = funcreturns(t1);
    
    require(TokenType::IDENTIFIER);
    
    std::string name = c.value;
    next(); // iden
    
    require(Symbol::PAREN_LEFT);
    next(); // (
    
    st_entry* entry = st()->add_or_get_empty_function(name);
    
    symbol_table* parent = st();
    push_context();
    ctx().st = st()->make_child(etable_owner::FUNCTION);
    auto cg = guard();
    
    if (thistype) {
        func.params.push_back({thistype, 0, "this"});
        st()->add_variable("this", thistype);
    }
    
    while (!is(Symbol::PAREN_RIGHT) && !is(TokenType::END_OF_FILE)) {
        func.params.push_back(parameter());
        
        if (is(Symbol::COMMA)) {
            next();
        } else {
            require(Symbol::PAREN_RIGHT);
        }
    }
    
    require(Symbol::PAREN_RIGHT);
    next(); // )
    
    func.ste = entry;
    
    entry = nullptr;
    
    type* ftype = types.get_or_add(functype);
    
    overload* ol = parent->get_overload(name, ftype, false);
    if (ol && ol->defined) {
        ast* ret = error("Function already defined");
        if (is(Symbol::BRACE_LEFT)) {
            panic(epanic_mode::ESCAPE_BRACE);
        }
        
        next();
        return ret;
    } else if (!ol) {
        ol = parent->add_function(name, ftype, nullptr, nullptr).second;
    }
    
    if (is(Symbol::BRACE_LEFT)) {
        push_context();
        ctx().function = ftype;
        auto cg = guard();
        
        ol->defined = true;
        ol->value = scope();
        delete ol->st;
        ol->st = st();
        
    } else {
        if (ol && !ol->defined) {
            require(Symbol::SEMICOLON);
            next(); // ;
            return error("Function already declared");
        }
        require(Symbol::SEMICOLON);
        next(); // ;
    }

    cg.deactivate();
    
    return ast::unary(Symbol::SYMDECL, ol->value, ftype, true, false); // Not owned
}

ast* parser::funcval() {
    type functype = type{ettype::FUNCTION};
    auto& func = functype.as_function();
    func.rets = funcreturns();
    
    std::string name{};
    
    if (is(TokenType::IDENTIFIER)) {
        name = c.value;
        next(); // iden
    }
    
    require(Symbol::PAREN_LEFT);
    next(); // (
    
    symbol_table* parent = st();
    push_context();
    ctx().st = st()->make_child(etable_owner::FUNCTION);
    auto cg = guard();
    
    st_entry* entry = st()->add_or_get_empty_function(name);
    
    /* if (thistype) {
        func.params.emplace_back(thistype, 0, "this");
        st()->add_variable("this", thistype);
    } */
    
    while (!is(Symbol::PAREN_RIGHT) && !is(TokenType::END_OF_FILE)) {
        func.params.push_back(parameter());
        
        if (is(Symbol::COMMA)) {
            next();
        } else {
            require(Symbol::PAREN_RIGHT);
        }
    }
    
    require(Symbol::PAREN_RIGHT);
    next(); // )
    
    std::vector<ast*> jailed{};
    u64 size{0};
    
    if (is(Symbol::BRACKET_LEFT)) {
        next(); // [
        while (!is(Symbol::BRACKET_RIGHT) && !is(TokenType::END_OF_FILE)) {
            ast* jail = ast::unary();
            auto& un = jail->as_unary();
            if (is(Symbol::AT)) {
                next(); // @
                un.op = Symbol::AT;
            } else {
                un.op = Symbol::ASSIGN;
            }
            ast* identifier = iden(false);
            auto& idn = identifier->as_symbol();
            if (!idn.symbol->is_variable()) {
                error("Can only capture variables", epanic_mode::IN_ARRAY);
                if (is(Symbol::COMMA)) {
                    next(); // ,
                }
                continue;
            }
            
            un.node = identifier;
            type* t{idn.symbol->as_variable().t};
            if (un.op == Symbol::AT) {
                type nt{ettype::POINTER, 0, etype_flags::CONST, type_pointer{eptr_type::NAKED, 0, t}};
                t = types.get_or_add(nt);
            }
            
            st()->add_variable(idn.name, t);
            size += t->get_size();
            
            jailed.push_back(jail);
            
            if (is(Symbol::COMMA)) {
                next(); // ,
            }
        }
        
        require(Symbol::BRACKET_RIGHT, epanic_mode::ESCAPE_BRACKET);
        next(); // ]
    }
    
    func.ste = entry;
    
    entry = nullptr;
    
    type* ftype = types.get_or_add(functype);
    
    overload* ol = st()->add_function(name, ftype, nullptr, nullptr).second;
    
    ast* scp{nullptr};
    
    if (is(Symbol::BRACE_LEFT)) {
        push_context();
        ctx().function = ftype;
        auto cg = guard();
        
        scp = scope();
    } else {
        error("Function values must be defined", epanic_mode::SEMICOLON);
    }
    
    cg.deactivate();
    
    ast** rj = new ast*[jailed.size()];
    std::memcpy(rj, jailed.data(), jailed.size() * sizeof(ast*));
    
    return ast::closure(ast::function(scp, ftype), rj, size);
}

parameter parser::nnparameter() {
    parameter ret{};
    param_flags flags{0};
    ast* typ = nntype();
    
    ret.t = typ->as_nntype().t;
    
    if (is(Symbol::SPREAD)) {
        next(); // ...
        flags |= eparam_flags::SPREAD;
    }
    
    if (is(TokenType::IDENTIFIER)) {
        ret.name = c.value;
        next(); // iden
    }
    
    if (is(Symbol::ASSIGN)) {
        next(); // =
        flags |= eparam_flags::DEFAULTABLE;
        
        push_context();
        ctx().expected = ret.t;
        auto cg = guard();
        
        ast* exp = aexpression();
        if(!exp->get_type()->can_weak_cast(ret.t)) {
            error("Cannot cast expression to parameter type");
        }
        ret.value = exp;
    }
    
    ret.flags = flags;
    
    return ret;
}

ast* parser::structdecl() {
    compiler_assert(Keyword::STRUCT);
    next(); // struct
    
    require(TokenType::IDENTIFIER);
    
    std::string name = c.value;
    next(); // iden
    
    require(Symbol::BRACE_LEFT);
    
    bool declared{false};
    
    type* stype{nullptr};
    
    st_entry* entry = st()->get(name, false);
    if (entry) {
        if (!entry->is_type() || entry->as_type().defined || !entry->as_type().t->is_struct()) {
            error("Struct already exists", epanic_mode::ESCAPE_BRACE);
            next(); // }
            return nullptr;
        }
        declared = true;
        stype = entry->as_type().t;
    } else {
        stype = types.add_type(type::_struct());
        entry = st()->add_type(name, stype, false);
        stype->as_struct().ste = entry;
    }
    
    type structtype = *stype;
    
    symbol_table* parent = st();
    push_context();
    entry->as_type().st = ctx().st = st()->make_child(etable_owner::STRUCT);
    ctx()._struct = &structtype;
    auto cg = guard();
    
    ast* scontent{nullptr};
    
    if (is(Symbol::BRACE_LEFT)) {
         scontent = structscope();
         types.update_type(stype->id, structtype);
    } else {
        if (declared) {
            require(Symbol::SEMICOLON);
            next(); // ;
            return error("Struct already declared");
        }
        require(Symbol::SEMICOLON);
        next(); // ;
    }

    cg.deactivate();
    
    return ast::unary(Symbol::SYMDECL, scontent, stype);
}

ast* parser::structscope() {
    compiler_assert(Symbol::BRACE_LEFT);
    next(); // {
    
    ast* block = ast::block(st());
    auto& stmts = block->as_block().stmts;
    
    while (!is(Symbol::BRACE_RIGHT) && !is(TokenType::END_OF_FILE)) {
        if (is(Keyword::USING)) {
            stmts.push_back(usingstmt());
        } else {
            stmts.push_back(structdeclstmt());
        }
    }
    
    require(Symbol::BRACE_RIGHT);
    
    return block;
}

ast* parser::uniondecl() {
    return nullptr;
}

ast* parser::unionscope() {
    return nullptr;
}

ast* parser::enumdecl() {
    return nullptr;
}

ast* parser::enumscope() {
    return nullptr;
}

ast* parser::assstmt() {
    return nullptr;
}

ast* parser::assignment() {
    return nullptr;
}

ast* parser::newinit() {
    return nullptr;
}

ast* parser::deletestmt() {
    return nullptr;
}

ast* parser::expressionstmt() {
    return nullptr;
}

ast* parser::expression() {
    return nullptr;
}

ast* parser::e17() {
    return nullptr;
}

ast* parser::e16() {
    return nullptr;
}

ast* parser::e15() {
    return nullptr;
}

ast* parser::e14() {
    return nullptr;
}

ast* parser::e13() {
    return nullptr;
}

ast* parser::e12() {
    return nullptr;
}

ast* parser::e11() {
    return nullptr;
}

ast* parser::e10() {
    return nullptr;
}

ast* parser::e9() {
    return nullptr;
}

ast* parser::e8() {
    return nullptr;
}

ast* parser::e7() {
    return nullptr;
}

ast* parser::e6() {
    return nullptr;
}

ast* parser::e5() {
    return nullptr;
}

ast* parser::e4() {
    return nullptr;
}

ast* parser::e3() {
    return nullptr;
}

ast* parser::e2() {
    return nullptr;
}

ast* parser::e1() {
    return nullptr;
}

ast* parser::ee() {
    return nullptr;
}

ast* parser::argument() {
    return nullptr;
}

ast* parser::fexpression(eexpression_type* expression_type) {
    return nullptr;
}

ast* parser::mexpression(eexpression_type* expression_type) {
    return nullptr;
}

ast* parser::aexpression() {
    return nullptr;
}

bool parser::is_type() {
    if (c.type == TokenType::IDENTIFIER) {
        auto entry = st()->get(c.value);
        return entry && entry->is_type();
    } else if (c.type == TokenType::KEYWORD) {
        Keyword kw = c.as_keyword();
        return kw == Keyword::BYTE   ||
               kw == Keyword::SHORT  ||
               kw == Keyword::INT    ||
               kw == Keyword::LONG   || 
               kw == Keyword::FLOAT  ||
               kw == Keyword::DOUBLE ||
               kw == Keyword::SIG    || 
               kw == Keyword::BOOL   ||
               kw == Keyword::CHAR   ||
               kw == Keyword::STRING || 
               kw == Keyword::FUN;
    } else {
        return false;
    }
}

bool parser::is_infer() {
    return is(Keyword::LET);
}

bool parser::is_decl_start() {
    return is_type() || is(Keyword::VOID) || is(Keyword::LET) || 
           is(Keyword::STRUCT) || is(Keyword::UNION) || is(Keyword::ENUM);
}

bool parser::is_compiler_token() {
    return is(TokenType::COMPILER_IDENTIFIER) || is(Symbol::COMPILER);
}

bool parser::is_assignment_operator() {
    if (c.type == TokenType::SYMBOL) {
        switch (c.as_symbol()) {
            case Symbol::ASSIGN:
            case Symbol::ADD_ASSIGN: [[fallthrough]];
            case Symbol::SUBTRACT_ASSIGN: [[fallthrough]];
            case Symbol::MULTIPLY_ASSIGN: [[fallthrough]];
            case Symbol::POWER_ASSIGN: [[fallthrough]];
            case Symbol::DIVIDE_ASSIGN: [[fallthrough]];
            case Symbol::AND_ASSIGN: [[fallthrough]];
            case Symbol::OR_ASSIGN: [[fallthrough]];
            case Symbol::XOR_ASSIGN: [[fallthrough]];
            case Symbol::SHIFT_LEFT_ASSIGN: [[fallthrough]];
            case Symbol::SHIFT_RIGHT_ASSIGN: [[fallthrough]];
            case Symbol::ROTATE_LEFT_ASSIGN: [[fallthrough]];
            case Symbol::ROTATE_RIGHT_ASSIGN: [[fallthrough]];
            case Symbol::CONCATENATE_ASSIGN: [[fallthrough]];
            case Symbol::BIT_SET_ASSIGN: [[fallthrough]];
            case Symbol::BIT_CLEAR_ASSIGN: [[fallthrough]];
            case Symbol::BIT_TOGGLE_ASSIGN: 
                return true;
            default:
                return false;
        }
    } else {
        return false;
    }
}

bool parser::is_pointer_type() {
    if (c.type == TokenType::SYMBOL) {
        switch (c.as_symbol()) {
            case Symbol::POINTER: [[fallthrough]];
            case Symbol::UNIQUE_POINTER: [[fallthrough]];
            case Symbol::SHARED_POINTER: [[fallthrough]];
            case Symbol::WEAK_POINTER:
                return true;
            default:
                return false;
        }
    } else {
        return false;
    }
}










