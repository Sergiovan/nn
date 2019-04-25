#include "frontend/parser.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <sstream>
#include <set>

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

parser::parser() : types(*(new type_table{})) {
    root_st = new symbol_table(etable_owner::FREE);
}

parser::parser(type_table& tt) : types(tt) {
    forked = true;
    root_st = new symbol_table(etable_owner::FREE); // TODO Fix this, oof
}

parser::~parser() {
    if (!forked) {
        delete root_st;
        delete &types; // Oofies
    }
}

parse_info parser::parse(lexer* l) {
    contexts.push({root_st, nullptr, nullptr, nullptr, nullptr, nullptr});
    parser::l = l;
    c = l->next();
    ast* res = _parse();
    return {res, root_st, &types, modules};
}

parse_info parser::parse(const std::string& str, bool is_file) {
    reader* r = (is_file ? reader::from_file : reader::from_string)(str);
    if (is_file) {
        file_path /= str;
    }
    lexer* l = new lexer{r};
    parse_info ret = parse(l);
    delete l; // Deletes r
    return ret;
}

symbol_table* parser::get_as_module() {
    root_st->set_owner(etable_owner::MODULE);
    return root_st;
}

parser parser::fork() {
    return parser{types}; 
}

bool parser::has_errors() {
    return !errors.empty();
}

void parser::print_errors() {
    for (auto& [t, e] : errors) {
        logger::error() << t.get_info() << " - " << e << logger::nend; 
    }
}

void parser::print_info() {
    logger::log() << "~~ Modules ~~\n\n";
    print_modules();
    logger::log() << "\n~~ Symbol table~~\n\n";
    print_st();
    logger::log() << "\n~~ Type table~~\n\n";
    print_types();
}

void parser::print_st() {
    logger::log() << root_st->print() << logger::nend;
}

void parser::print_modules() {
    for (auto& [n, m] : modules) {
        logger::log() << n << ":\n" << m->print() << logger::nend;
    }
}

void parser::print_types() {
    logger::log() << types.print() << logger::nend;
}

ast* parser::_parse() {
    ast* ret = program();
    if (unfinished.size()) {
        finish();
    }
    return ret;
}

void parser::finish() {
    for (auto& [val, ctx] : unfinished) {
        if (val->is_unary() && val->as_unary().op == Symbol::KWGOTO) {
            auto& unn = val->as_unary();
            auto& str = unn.node->as_string();
            std::string labelname((char*) str.chars, str.length);
            if (auto label = labels.find(labelname); label != labels.end()) {
                delete unn.node;
                unn.node = label->second;
            } else {
                error("Unable to finish GOTO element");
            }
        } else {
            error("Unable to finish element");
        }
    }
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
    return ast::none(); // In case we want to make an error AST or stub
}

ast* parser::operator_error(Symbol op, type* t, bool post) {
    std::stringstream ss{};
    ss << "Cannot use operator '" << symbol_names.at(op) << "' on type \"" << t->print(true) << "\"";
    return error(ss.str());
}

ast* parser::operator_error(Symbol op, type* l, type* r) {
    if (op != Symbol::ASSIGN) {
        std::stringstream ss{};
        ss << "Cannot use operator '" << symbol_names.at(op) << "' on types \"" << l->print(true) << 
            "\" and \"" << r->print(true) << "\"";
        return error(ss.str());
    } else {
        std::stringstream ss{};
        ss << "Cannot cast \"" << r->print(true) << "\" to \"" << l->print(true) << "\" implicitly";
        return error(ss.str());
    }
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
    if constexpr (__debug) {
        // logger::debug() << "Token \"" << c.value << '"' << logger::nend;
    }
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
            ss << "Expected \"" << Grammar::tokentype_names.at(tt) << " but got \"" << c.value << "\" instead,";
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
            ss << "Expected symbol \"" << Grammar::symbol_names.at(sym) << "\" but got \"" << c.value << "\" instead";
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
            ss << "Expected keyword \"" << Grammar::keyword_names.at(kw) << " but got \"" << c.value << "\" instead";
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

u64 parser::peek_until(TokenType tt, bool skip_groups) {
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
    return amount;
}

u64 parser::peek_until(Symbol sym, bool skip_groups) {
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
    return amount;
}

u64 parser::peek_until(const std::vector<Grammar::Symbol>& syms, bool skip_groups) {
    if (syms.empty()) {
        throw parser_exception{};
    }
    u64 amount = 0;
    
    token t = l->peek(amount);
    while (t.type != TokenType::END_OF_FILE && 
          (t.as_symbol() == Symbol::SYMBOL_INVALID || std::find(syms.begin(), syms.end(), t.as_symbol()) == syms.end())) {
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
    return amount;
}

u64 parser::peek_until(Keyword kw, bool skip_groups) {
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
    return amount;
}

token parser::skip_until(TokenType tt, bool skip_groups) {
    return skip(peek_until(tt, skip_groups));
}

token parser::skip_until(Symbol sym, bool skip_groups) {
    return skip(peek_until(sym, skip_groups));
}

token parser::skip_until(const std::vector<Grammar::Symbol>& syms, bool skip_groups) {
    return skip(peek_until(syms, skip_groups));
}

token parser::skip_until(Keyword kw, bool skip_groups) {
    return skip(peek_until(kw, skip_groups));
}

ast* parser::iden(bool withthis, type** thistype) {
    if (!is(Keyword::THIS)) {
        require(TokenType::IDENTIFIER);
    }
    
    auto tok = next(); // iden
    auto sym = ctx().st->get(tok.value);
    if (!sym && withthis) {
        sym = ctx().st->get("this");
        if (sym && sym->is_variable()) {
            type* t = sym->get_type();
            auto self = sym;
            if (t->is_pointer(eptr_type::NAKED)) {
                t = t->as_pointer().t;
            }
            if (thistype) {
                *thistype = t;
            }
            if (t->is_struct()) {
                sym = t->as_struct().ste->as_type().st->get(tok.value);
            } else if (t->is_union()) {
                sym = t->as_union().ste->as_type().st->get(tok.value);
            }
            if (sym && (t->is_struct() || t->is_union())) {
                if (sym->is_function()) {
                    if (peek(Symbol::PAREN_LEFT)) {
                        ctx().first_param = ast::symbol(self);
                    } else {
                        error("Method access on values or variables must be used as function calls");
                    }
                } else if (sym->is_field()) {
                    return ast::binary(Symbol::ACCESS, ast::symbol(self), ast::symbol(sym));
                }
            }
        } else {
            sym = nullptr;
        }
    }
    
    if (!sym) {
        std::stringstream ss{};
        ss << '"' << tok.value << '"' << "does not exist";
        error(ss.str(), epanic_mode::NO_PANIC, &tok);
        return ast::none();
    }
    return ast::symbol(sym);
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
    u64 val = next().as_integer();
    if (val <= 0xFF) {
        return ast::byte(val, types.t_byte);
    } else if (val <= 0xFFFF) {
        return ast::word(val, types.t_short);
    } else if (val <= 0xFFFFFFFF) {
        return ast::dword(val, types.t_int);
    } else {
        return ast::qword(val, types.t_long);
    }
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
    
    ast* ret = ast::array(new ast*[elems.size()], elems.size(), ctx().expected);
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
            if (!exp->get_type()->can_weak_cast(s.fields[field].t)) {
                exp = error("Cannot cast to correct type", epanic_mode::IN_STRUCT_LIT);
            }
            elems[field] = exp;
            
        } else {
            push_context();
            ctx().expected = s.fields[field].t;
            auto cg = guard();
            
            ast* exp = aexpression();
            if (!exp->get_type()->can_weak_cast(s.fields[field].t)) {
                exp = error("Cannot cast to correct type", epanic_mode::IN_STRUCT_LIT);
            }
            elems[field] = exp;
            ++field;
        }
        
    } while (is(Symbol::COMMA));
    require(Symbol::BRACE_RIGHT, epanic_mode::ESCAPE_BRACE);
    
    next(); // }
    
    ret->as_struct().elems = new ast*[elems.size()];
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
        error("Invalid literal");
        return ast::none();
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
    } else if (is(Keyword::DELETE)) {
        ret = deletestmt();
    } else if (is_decl_start()) {
        ret = freedeclstmt();
    } else if (is(Symbol::BRACE_LEFT)) {
        ret = scope(etable_owner::BLOCK);
    } else {
        ret = mexpression();
        require(Symbol::SEMICOLON, epanic_mode::SEMICOLON);
        next(); // ;
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
    } else if (is(Keyword::DELETE)) {
        ret = deletestmt();
    } else if (is(Symbol::BRACE_LEFT)) {
        ret = scope();
    } else {
        ret = mexpression();
        require(Symbol::SEMICOLON, epanic_mode::SEMICOLON);
        next(); // ;
    }
    
    return ret;
}

ast* parser::scope(etable_owner from) {
    compiler_assert(Symbol::BRACE_LEFT);
    
    next(); // {
    
    push_context();
    ctx().st = st()->make_child(from);
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
        if (is(Symbol::SEMICOLON)) {
            next(); // ;
        }
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
    ctx().st = st()->make_child(etable_owner::LOOP);
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
            
            push_context();
            ctx().expected = new_iden->get_type();
            auto cg = guard();
            
            ast* idenass = vardeclass();
            if (is(Symbol::BRACE_LEFT) || is(Keyword::DO)) {
                ftype = fortype::LUA;
            } else {
                ftype = fortype::CLASSIC;
            }
            
            if (new_iden->is_binary() && new_iden->as_binary().op == Symbol::SYMDECL) {
                for (auto& stmt : new_iden->as_binary().left->as_block().stmts) {
                    auto& sym = stmt->as_symbol();
                    st()->add(sym.get_name(), sym.symbol);
                }
                new_iden->as_binary().right = idenass;
                start = new_iden;
            } else {
                start = ast::binary(Symbol::ASSIGN, new_iden, idenass);
            }
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
    
    ast* ret{nullptr};
    
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
                next(); // ;
                do {
                    stmts.push_back(mexpression());
                    if (is(Symbol::SEMICOLON)) {
                        next(); // ;
                    }
                } while (!is(Symbol::BRACE_LEFT) && !is(Keyword::DO) && !is(TokenType::END_OF_FILE));
            }
            
            ret = ast::unary(Symbol::KWFORCLASSIC, parts);
            break;
        }
        case fortype::FOREACH: { // for x : y { }
            compiler_assert(Symbol::COLON);
            next(); // :
            ast* colon = expression();
            type* ct = colon->get_type();
            
            if (!ct->is_pointer(eptr_type::ARRAY) && !ct->is_primitive(etype_ids::STRING)) {
                std::stringstream ss{};
                ss << "Cannot use for-each on \"" << ct->print(true) << "\"";
                colon = error(ss.str());
            } else if (ct->is_pointer(eptr_type::ARRAY) && !ct->as_pointer().t->can_weak_cast(start->get_type())) {
                std::stringstream ss{};
                ss << "Cannot cast \"" << ct->as_pointer().t->print(true) << "\" to \"" << start->get_type()->print(true) << "\" in foreach";
                colon = error(ss.str());
            } else if (ct->is_primitive(etype_ids::STRING) && !types.t_char->can_weak_cast(start->get_type())) {
                std::stringstream ss{};
                ss << "Cannot cast \"" << ct->print(true) << "\" to \"" << start->get_type()->print(true) << "\" in foreach";
                colon = error(ss.str());
            }
            
            if (start->is_binary() && start->as_binary().op == Symbol::SYMDECL) {
                for (auto& stmt : start->as_binary().left->as_block().stmts) {
                    auto& sym = stmt->as_symbol();
                    st()->add(sym.get_name(), sym.symbol);
                }
                start->as_binary().right = colon;
            }
            
            ret = ast::unary(Symbol::KWFOREACH, start);
            
            break;
        }
        case fortype::LUA: { // for x = y, z, w { }
            auto& luablock = start->as_binary();
            auto& commas   = luablock.right->as_block();
            auto len = commas.stmts.size();
            if (len < 2 || len > 3) {
                error("Lua for had an illegal number of values");
            }
            ret = ast::unary(Symbol::KWFORLUA, start);
            break;
        }
        case fortype::INVALID: [[fallthrough]];
        default:
            return error("Invalid for type", epanic_mode::ULTRA_PANIC);
    }
    
    return ret;
}

ast* parser::whilestmt() {
    compiler_assert(Keyword::WHILE);
    next(); // while
    
    push_context();
    ctx().st = st()->make_child(etable_owner::LOOP);
    auto cg = guard();
    
    ast* conditions = ast::block(st());
    auto& stmts = conditions->as_block().stmts;
    
    eexpression_type last{eexpression_type::INVALID};
    do {
        if (is(Symbol::SEMICOLON)) {
            next(); // ;
        }
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
    ctx().st = st()->make_child(etable_owner::LOOP);
    auto cg = guard();
    
    ast* conditions = ast::block(st());
    auto& stmts = conditions->as_block().stmts;
    
    eexpression_type last{eexpression_type::INVALID};
    do {
        if (is(Symbol::SEMICOLON)) {
            next(); // ;
        }
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
        bin.left = ast::none();
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
        next(); // continue
        bin.right = ast::unary(Symbol::KWCONTINUE);
        require(Symbol::SEMICOLON, epanic_mode::SEMICOLON);
        next(); // ;
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
    
    next(); // catch
    
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
        if (rtype.as_combination().types.size() == 1) {
            un.t = rtype.as_combination().types[0];
        } else {
            un.t = types.get_or_add(rtype);
        }
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
                        un.node = operator_error(Symbol::ASSIGN, rcomb.types[i], uncomb.types[j]);
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
                    un.node = operator_error(Symbol::ASSIGN, rcomb.types[i], un.t);
                }
                ++i;
                while (i < rcomb.types.size() && rcomb.types[i] == types.t_sig) {
                    ++i;
                }
                if (i != rcomb.types.size()) {
                    std::stringstream ss{};
                    ss << "Only " << i << " return type(s) given when " << rcomb.types.size() << " were needed";
                    un.node = error(ss.str());
                }
            }
        } else {
            if (!un.t->can_weak_cast(ret_type)) {
                un.node = operator_error(Symbol::ASSIGN, ret_type, un.t);
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
                overload_st->add_field(c.value, val, ctx().function);
            } else {
                val = sigval->as_field().field;
            }
            ret = ast::unary(Symbol::KWRAISE, ast::qword(val, types.t_sig));
            next(); // sig name
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
    
    ast* ret = ast::unary(Symbol::KWGOTO, nullptr);
    std::string labelname = c.value;
    if (auto label = labels.find(labelname); label == labels.end()) {
        ret->as_unary().node = ast::string(c.value);
        unfinished.push_back({ret, ctx()});
    } else {
        ret->as_unary().node = label->second;
    }
    
    next(); // iden
    require(Symbol::SEMICOLON, epanic_mode::SEMICOLON);
    next(); // ;
    return ret;
}

ast* parser::labelstmt() {
    compiler_assert(Keyword::LABEL);
    next(); // label
    
    ast* ret{nullptr};
    
    if (labels.find(c.value) != labels.end()) {
        ret = error("Label already exists");
    } else {
        st_entry* label = st()->add_label(c.value);
        ast* labelast{nullptr};
        if (!label) {
            labelast = error("Object with that name already exists ");
        } else {
            labelast = ast::symbol(label);
        }
        ret = ast::unary(Symbol::KWLABEL, ast::symbol(label));
        labels.insert({c.value, ret});
    }
    
    next(); // iden
    require(Symbol::SEMICOLON, epanic_mode::SEMICOLON);
    next(); // ;
    return ret;
}

ast* parser::deferstmt() {
    compiler_assert(Keyword::DEFER);
    next(); // defer
    
    ast* stmt = statement(); // Semicolon handled by statement();
    return ast::unary(Symbol::KWDEFER, stmt);
}

ast* parser::breakstmt() {
    compiler_assert(Keyword::BREAK);
    next(); // break
    
    symbol_table* st = ctx().st;
    bool in_loop{false};
    while (st) {
        if (st->owned_by(etable_owner::LOOP)) {
            in_loop = true;
            break;
        } else if (st->owned_by(etable_owner::FUNCTION)) {
            in_loop = false;
            break;
        }
        st = st->parent;
    }
    
    if (!in_loop) {
        error("Cannot use break outside of loop");
    }
    
    require(Symbol::SEMICOLON, epanic_mode::SEMICOLON);
    next(); // ;
    return ast::unary(Symbol::KWBREAK);
}

ast* parser::continuestmt() {
    compiler_assert(Keyword::CONTINUE);
    next(); // continue
    
    symbol_table* st = ctx().st;
    bool in_loop{false};
    while (st) {
        if (st->owned_by(etable_owner::LOOP)) {
            in_loop = true;
            break;
        } else if (st->owned_by(etable_owner::FUNCTION)) {
            in_loop = false;
            break;
        }
        st = st->parent;
    }
    
    if (!in_loop) {
        error("Cannot use continue outside of loop");
    }
    
    require(Symbol::SEMICOLON, epanic_mode::SEMICOLON);
    next(); // ;
    return ast::unary(Symbol::KWCONTINUE);
}

ast* parser::leavestmt() {
    compiler_assert(Keyword::LEAVE);
    next(); // leave
    
    if (st()->owned_by(etable_owner::FUNCTION)) {
        error("Cannot leave function, use 'return' instead");
    }
    
    require(Symbol::SEMICOLON, epanic_mode::SEMICOLON);
    next(); // ;
    return ast::unary(Symbol::KWLEAVE);
}

ast* parser::importstmt() {
    using namespace std::string_literals;
    namespace fs = std::filesystem;
    
    compiler_assert(Keyword::IMPORT);
    next(); // import
    
    ast* ret{nullptr}; 
    
    ast* what{nullptr};
    std::string as{};
    fs::path p{file_path.parent_path()};
    bool asterisk{false};
    
    if (is(TokenType::STRING)) {
        p /= c.value;
        if (fs::exists(p) && fs::is_regular_file(p)) {
            p = fs::canonical(p);
            what = ast::string(p.string());
        } else {
            what = error("Path does not exist or is not a file");
        }
    } else {
        std::stringstream path{};
        path << c.value;
        as = c.value;
        next(); // iden
        while (is(Symbol::DOT)) {
            if (asterisk) {
                error("Cannot have more names after asterisk", epanic_mode::SEMICOLON);
                break;
            }
            
            p /= as;
            if (!fs::exists(p) || !fs::is_directory(p)) {
                error("Path does not exist");
            }
            path << c.value;
            next(); // .
            if (!is(TokenType::IDENTIFIER)) {
                require(Symbol::ASTERISK, epanic_mode::SEMICOLON);
                asterisk = true;
            }
            path << c.value;
            if (!asterisk) {
                as = c.value;
            }
            next(); // iden *
        }
        p /= (as + ".nn"s);
        if (fs::exists(p) && fs::is_regular_file(p)) { // TODO Check we're not importing ourselves
            p = fs::canonical(p);
            what = ast::string(p.string());
        } else {
            what = error("Path does not exist or is not a file"); 
        }
    }
    
    if (is(Keyword::AS)) {
        next(); // as
        if (st()->get(c.value)) {
            error("Identifier already exists");
        } else {
            as = c.value;
        }
        next(); // iden
    }
    
    require(Symbol::SEMICOLON, epanic_mode::SEMICOLON);
    next(); // ;
    
    std::string module_name{p.string()};
    symbol_table* mod{nullptr};
    
    if (auto stored_module = modules.find(module_name); stored_module == modules.end()) {
        parser pars = fork();
        ast* module = pars.parse(module_name, true).result;
        if (pars.has_errors()) {
            errors.insert(errors.end(), pars.errors.begin(), pars.errors.end());
        }
        for (auto& [name, val] : pars.modules) {
            if (modules.find(name) == modules.end()) {
                modules.insert({name, val});
            }
        }
        mod = pars.get_as_module();
        modules.insert({p.string(), mod});
    } else {
        mod = stored_module->second;
    }
    
    if (asterisk) {
        st()->merge_st(mod);
        ret = ast::binary(Symbol::KWIMPORT, what, ast::none());
    } else {
        st_entry* sym = st()->add_module(as, mod);
        ret = ast::binary(Symbol::KWIMPORT, what, ast::symbol(sym));
    }
    
    return ret;
}

ast* parser::usingstmt() {
    using namespace std::string_literals;
    compiler_assert(Keyword::USING);
    
    std::vector<std::string> path{};
    std::string as{};
    bool true_as{false};
    bool asterisk = false;
    
    do {
        if (asterisk) {
            error("Cannot have more names after asterisk", epanic_mode::SEMICOLON);
            break;
        }
        
        next(); // using, .
        if(!is(TokenType::IDENTIFIER)) {
            require(Symbol::ASTERISK);
            asterisk = true;
        } else {
            path.push_back(c.value);
        }
        next(); //iden *
        
    } while(is(Symbol::DOT));
    
    if (is(Keyword::AS)) {
        if (asterisk) {
            error("Cannot give name to asterisk expression", epanic_mode::SEMICOLON);
        } else {
            true_as = true;
            next(); // as
            require(TokenType::IDENTIFIER);
            as = c.value;
            next();
        }
    } else {
        as = path.back();
    }
    
    require(Symbol::SEMICOLON, epanic_mode::SEMICOLON);
    next(); // ;
    
    symbol_table* cur{st()};
    bool first = true;
    bool last = false;
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
            if (!entry->as_type().st) {
                last = true;
            } else {
                cur = entry->as_type().st;
            }
        } else if (entry->is_function()) {
            cur = entry->as_function().st;
        } else if (entry->is_namespace() || entry->is_module()) {
            cur = entry->as_namespace().st;
        } else {
            return error("Cannot use element found");
        }
    }
    
    if (asterisk) {
        st()->merge_st(cur);
    } else {
        if (as.empty()) {
            as = entry->name;
        }
        st()->borrow(as, entry);
    }
    
    return ast::unary(Symbol::KWUSING, ast::symbol(entry));
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
    
    while (!is(Symbol::BRACE_RIGHT) && !is(TokenType::END_OF_FILE)) {
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
    
    require(Symbol::BRACE_RIGHT, epanic_mode::ESCAPE_BRACE);
    next(); 
    
    return ns;
}

type_flags parser::typemod(bool* any) {
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
        if (any) {
            *any = true;
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

ast* parser::nntype(bool* endswitharray) {
    ast* ret = ast::nntype();
    auto& nnt = ret->as_nntype();
    type* t{nullptr};
    if (endswitharray) {
        *endswitharray = false;
    }
    
    if (is(TokenType::KEYWORD)) {
        switch (c.as_keyword()) {
            case Keyword::BYTE:
                t = types.t_byte;
                next(); // byte
                break;
            case Keyword::SHORT:
                t = types.t_short;
                next(); // short
                break;
            case Keyword::INT:
                t = types.t_int;
                next(); // int
                break;
            case Keyword::LONG:
                t = types.t_long;
                next(); // long
                break;
            case Keyword::SIG:
                t = types.t_sig;
                next(); // sig
                break;
            case Keyword::FLOAT:
                t = types.t_float;
                next(); // float
                break;
            case Keyword::DOUBLE:
                t = types.t_double;
                next(); // double
                break;
            case Keyword::BOOL:
                t = types.t_bool;
                next(); // bool
                break;
            case Keyword::CHAR:
                t = types.t_char;
                next(); // char
                break;
            case Keyword::STRING:
                t = types.t_string;
                next(); // string
                break;
            case Keyword::FUN: {
                ast* ft = functype();
                t = ft->as_nntype().t;
                break;
            }
            default:
                nnt.t = types.t_void;
                error("Invalid type", epanic_mode::SEMICOLON);
                return ret;
        }
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
    
    bool mods{false};
    type_flags flags = typemod(&mods);
    
    type constructed = *t;
    
    if (mods) {
        constructed.flags = flags;
        t = types.get_or_add(constructed);
    }
    
    
    while (is_pointer_type() || is(Symbol::BRACKET_LEFT)) {
        constructed = type{ettype::POINTER};
        constructed.as_pointer().t = t;
        
        if (is(Symbol::BRACKET_LEFT)) {
            constructed.as_pointer().ptr_t = eptr_type::ARRAY;
            ast* size = arraysize();
            if (!size->get_type()->can_weak_cast(types.t_long) && size->get_type() != types.t_void) {
                size = error("Array size must be a long type");
            } 
            // TODO Const expression?
            nnt.array_sizes.push_back(size);
            if (endswitharray) {
                *endswitharray = true;
            }
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
            next(); // * *+ *! *?
            if (endswitharray) {
                *endswitharray = false;
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
    
    if (is(Keyword::VOID)) {
        next(); // void
        frtype.as_combination().types.push_back(types.t_void);
        require(Symbol::PAREN_LEFT);
    } else {
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
    }
    
    type* r{nullptr};
    if (frtype.as_combination().types.size() == 1) {
        r = frtype.as_combination().types[0];
    } else {
        r = types.get_or_add(frtype);
    }
    
    ftype.as_pfunction().rets = r;
    auto& params = ftype.as_pfunction().params;
    
    require(Symbol::PAREN_LEFT);    
    next(); // (
    
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
            next(); // let
            break;
        default:
            t = types.t_void;
            next();
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
        ast* t1{nullptr};
        if (is_infer()) {
            t1 = infer();
        } else {
            t1 = nntype();
        }
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
        type* thistype = pointer_to(ctx()._struct);
        if (is(Keyword::VOID)) {
            return funcdecl(nullptr, thistype);
        }
        ast* t1{nullptr};
        if (is_infer()) {
            t1 = infer();
        } else {
            t1 = nntype();
        }
        if (is(Symbol::COLON)) {
            return funcdecl(t1, thistype);
        } else if (peek(Symbol::PAREN_LEFT)) {
            return funcdecl(t1, thistype);
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
        ast* t1{nullptr};
        if (is_infer()) {
            t1 = infer();
        } else {
            t1 = nntype();
        }
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
        std::vector<type*> rhtypes{};
        
        u64 i{0};
        
        push_context();
        auto cg = guard();
        
        while (true) {
            if (i >= types.size()) {
                error("Too many expressions for assignment", epanic_mode::SEMICOLON);
                break;
            }
            ctx().expected = types[i];
            ast* exp = aexpression();
            type* exptype = exp->get_type();
            if (exptype->is_combination()) {
                auto& ctypes = exptype->as_combination().types;
                rhtypes.insert(rhtypes.end(), ctypes.begin(), ctypes.end());
                i += ctypes.size();
            } else {
                rhtypes.push_back(exptype);
                ++i;
            }
            stmts.push_back(exp);
            
            if (is(Symbol::SEMICOLON)) {
                break;
            } else {
                if (!require(Symbol::COMMA, epanic_mode::SEMICOLON)) {
                    break;
                }
            }
        }
        
        for (u64 i = 0; i < types.size(); ++i) {
            if (i >= rhtypes.size()) {
                break;
            }
            if (!rhtypes[i]->can_weak_cast(types[i])) {
                error("Cannot cast assignment expression to correct type");
            }
        }
        
    } else {
        ast* exp = aexpression();
        type* exptype = exp->get_type();
        type* t = exptype->is_combination() ? exptype->as_combination().types[0] : exptype;
        if (!t->can_weak_cast(e)) {
            error("Cannot cast assignment expression to correct type");
        }
        stmts.push_back(exp);
        if (is(Symbol::COMMA)) {
            error("Too many expressions for assignment", epanic_mode::SEMICOLON);
        }
    }
    
    /* if (e->is_combination()) {
        auto& types = e->as_combination().types;
        
        push_context();
        auto cg = guard();
        
        for (type* typ : types) {
            ctx().expected = typ;
            ast* exp = aexpression();
            if (!exp->get_type()->can_weak_cast(typ)) {
                error("Cannot cast assignment expression to correct type");
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
        bool first = true;
        do {
            if (first) {
                first = false;
            } else {
                next(); // ,
            }
            ast* exp = aexpression();
            if (!exp->get_type()->can_weak_cast(e)) {
                error("Cannot cast assignment expression to correct type");
            }
            stmts.push_back(exp);
        } while (is(Symbol::COMMA));
    } */
    
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
    st_entry* entry{st_entry::variable(name, expected)};
    
    stmts.push_back(ast::symbol(entry));
    
    if (is(Symbol::COMMA)) {
        type comb = type{ettype::COMBINATION};
        auto& types = comb.as_combination().types;
        types.push_back(expected);
    
        while (is(Symbol::COMMA)) {
            next(); // ,
            
            if (is_infer()) {
                delete typ;
                typ = infer();
            } else if (is_type()) {
                delete typ;
                typ = nntype();
            }
            
            require(TokenType::IDENTIFIER);
            name = c.value;
            next(); // iden
            
            expected = typ->as_nntype().t;
            entry = st_entry::variable(name, expected);
            stmts.push_back(ast::symbol(entry));
            types.push_back(expected);
        }
        
        expected = parser::types.get_or_add(comb);
    }
    
    delete typ;
    typ = nullptr;
    
    return ast::binary(Symbol::SYMDECL, ret, nullptr, expected);
}

ast* parser::freevardecl(ast* t1) {
    ast* iden = freevardecliden(t1);
    
    if (is(Symbol::ASSIGN)) {
        push_context();
        ctx().expected = iden->as_binary().t;
        auto cg = guard();
        
        auto& stmts = iden->as_binary().left->as_block().stmts;
        
        ast* values = vardeclass();
        auto& vals = values->as_block().stmts;
    
        iden->as_binary().right = values;
        
        std::vector<std::pair<ast*, type*>> value_types{};
        for (auto& val : vals) {
            if (val->get_type()->is_combination()) {
                for (type* t : val->get_type()->as_combination().types) {
                    value_types.push_back({val, t}); // TODO indexing of value
                }
            } else {
                value_types.push_back({val, val->get_type()});
            }
        }
        
        for (u64 i = 0; i < stmts.size(); ++i) {
            auto& sym = stmts[i]->as_symbol();
            st()->add(sym.get_name(), sym.symbol);
            
            if (i < value_types.size()) {
                auto& [val, typ] = value_types[i];
                auto& var = sym.symbol->as_variable();
                var.value = val;
                if (var.t == types.t_let) {
                    var.t = typ;
                }
                var.defined = true;
            } else {
                if (stmts[i]->as_symbol().symbol->as_variable().t == types.t_let) {
                    error("Variable has uninferrable type");
                }
            }
        }
    } else {
        auto& stmts = iden->as_binary().left->as_block().stmts;
        for (auto& stmt : stmts) {
            auto& sym = stmt->as_symbol();
            st()->add(sym.get_name(), sym.symbol);
            if (sym.symbol->as_variable().t == types.t_let) {
                error("Variable has uninferrable type");
            }
        }
        iden->as_binary().right = ast::none();
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
    st_entry* entry{st_entry::field(name, fields.size(), ctx()._struct)};
    
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
            next(); // value
        }
    }
    
    fields.push_back(f);
    
    stmts.push_back(ast::symbol(entry));
    
    if (is(Symbol::COMMA)) {
        type comb = type{ettype::COMBINATION};
        auto& types = comb.as_combination().types;
        types.push_back(expected);
    
        while (is(Symbol::COMMA)) {
            next(); // ,
            
            if (is_infer()) {
                delete typ;
                typ = infer();
            } else if (is_type()) {
                delete typ;
                typ = nntype();
            }
            
            require(TokenType::IDENTIFIER);
            name = c.value;
            next(); // iden
            
            expected = typ->as_nntype().t;
            entry = st_entry::field(name, fields.size(), ctx()._struct);
            
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
            
            stmts.push_back(ast::symbol(entry));
            types.push_back(expected);
        }
        
        expected = parser::types.get_or_add(comb);
    }
    
    delete typ;
    
    return ast::binary(Symbol::SYMDECL, ret, nullptr, expected);
}

ast* parser::structvardecl(ast* t1) {
    ast* iden = structvardecliden(t1);
    
    if (is(Symbol::ASSIGN)) {
        push_context();
        ctx().expected = iden->as_binary().t;
        auto cg = guard();
        
        auto& stmts = iden->as_binary().left->as_block().stmts;
        
        ast* values = vardeclass();
        auto& vals = values->as_block().stmts;
        iden->as_binary().right = values;
        
        std::vector<std::pair<ast*, type*>> value_types{};
        for (auto& val : vals) {
            if (val->get_type()->is_combination()) {
                for (type* t : val->get_type()->as_combination().types) {
                    value_types.push_back({val, t}); // TODO indexing of value
                }
            } else {
                value_types.push_back({val, val->get_type()});
            }
        }
        
        auto& fields = ctx()._struct->as_struct().fields;
        
        for (u64 i = 0; i < stmts.size(); ++i) {
            auto& sym = stmts[i]->as_symbol();
            st()->add(sym.get_name(), sym.symbol);
            
            if (i < value_types.size()) {
                auto& [val, typ] = value_types[i];
                auto& var = sym.symbol->as_field();
                fields[var.field].value = val;
                if (fields[var.field].t == types.t_let) {
                    fields[var.field].t = typ;
                }
            } else {
                auto& var = stmts[i]->as_symbol().symbol->as_field();
                if (fields[var.field].t == types.t_let) {
                    error("Variable has uninferrable type");
                }
            }
        }
    } else {
        auto& fields = ctx()._struct->as_struct().fields;
        auto& stmts = iden->as_binary().left->as_block().stmts;
        for (auto& stmt : stmts) {
            auto& sym = stmt->as_symbol();
            st()->add(sym.get_name(), sym.symbol);
            if (fields[sym.symbol->as_field().field].t == types.t_let) {
                error("Variable has uninferrable type");
            }
        }
        iden->as_binary().right = ast::none();
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
    
    ufield f;
    f.t = expected;
    f.name = name;
    
    fields.push_back(f);
    
    ast* exp{nullptr};
    if (is(Symbol::ASSIGN)) {
        next(); // =
        
        push_context();
        ctx().expected = expected;
        auto cg = guard();
        
        exp = aexpression();
        type* exptype = exp->get_type();
        if (exptype->is_combination()) {
            exptype = exptype->as_combination().types[0];
        }
        if (!exptype->can_weak_cast(expected)) {
            error("Cannot cast assignment expression to correct type");
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
    } else {
        if (expected == types.t_let) {
            error("Variable has uninferrable type");
        }
    }
    
    st_entry* entry = st()->add_field(name, fields.size() - 1, ctx()._struct);
    ast* ret = ast::symbol(entry);
    
    return ast::binary(Symbol::SYMDECL, ret, exp ? exp : ast::none(), expected);
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
    ctx().st = parent->make_child(etable_owner::FUNCTION);
    auto cg = guard();
    
    if (thistype) {
        func.params.push_back({thistype, 0, "this"});
        st()->add_variable("this", thistype);
    }
    
    bool defaulted{false};
    bool last{false};
    
    while (!is(Symbol::PAREN_RIGHT) && !is(TokenType::END_OF_FILE)) {
        if (last) {
            error("Spread parameter must be last");
        }
        
        auto p = nnparameter();
        func.params.push_back(p);
        st()->add_variable(p.name, p.t);
        
        if (p.flags & eparam_flags::DEFAULTABLE) {
            defaulted = true;
        }
        
        if (defaulted && !(p.flags & eparam_flags::DEFAULTABLE)) {
            error("Already declared default parameter");
        }
        
        if (p.flags & eparam_flags::SPREAD) {
            last = true;
        }
        
        if (is(Symbol::COMMA)) {
            next();
        } else {
            require(Symbol::PAREN_RIGHT);
        }
    }
    
    require(Symbol::PAREN_RIGHT);
    next(); // )
    
    func.ste = entry;
    
    type* ftype = types.get_or_add(functype);
    
    overload* ol = parent->get_overload(name, ftype, false);
    bool declared = ol != nullptr;
    
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
        ol->value = scope(etable_owner::FUNCTION);
        delete ol->st;
        ol->st = st();
        
    } else {
        if (ol && !ol->defined && declared) {
            require(Symbol::SEMICOLON);
            next(); // ;
            return error("Function already declared");
        }
        require(Symbol::SEMICOLON);
        next(); // ;
    }

    cg.deactivate();
    
    return ast::binary(Symbol::SYMDECL, ast::symbol(entry), ol && ol->value ? ol->value : ast::none(), ftype);
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
    
    bool defaulted{false};
    bool last{false};
    
    while (!is(Symbol::PAREN_RIGHT) && !is(TokenType::END_OF_FILE)) {
        if (last) {
            error("Spread parameter must be last");
        }
        
        auto p = nnparameter();
        func.params.push_back(p);
        st()->add_variable(p.name, p.t);
        
        if (p.flags & eparam_flags::DEFAULTABLE) {
            defaulted = true;
        }
        
        if (defaulted && !(p.flags & eparam_flags::DEFAULTABLE)) {
            error("Already declared default parameter");
        }
        
        if (p.flags & eparam_flags::SPREAD) {
            last = true;
        }
        
        if (is(Symbol::COMMA)) {
            next();
        } else {
            require(Symbol::PAREN_RIGHT);
        }
    }
    
    require(Symbol::PAREN_RIGHT);
    next(); // )
    
    std::vector<ast*> jailed{};
    
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
            
            st()->add_variable(idn.get_name(), t);
            
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
        
        scp = scope(etable_owner::FUNCTION);
    } else {
        error("Function values must be defined", epanic_mode::SEMICOLON);
    }
    
    cg.deactivate();
    
    ast** rj = new ast*[jailed.size()];
    std::memcpy(rj, jailed.data(), jailed.size() * sizeof(ast*));
    
    return ast::closure(ast::function(scp, ftype), rj, jailed.size());
}

parameter parser::nnparameter() {
    parameter ret{};
    param_flags flags{0};
    ast* typ = nntype();
    
    ret.t = typ->as_nntype().t;
    
    if (is(Symbol::SPREAD)) {
        next(); // ...
        flags |= eparam_flags::SPREAD;
        ret.t = pointer_to(ret.t, eptr_type::ARRAY);
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
    
    symbol_table* parent = st();
    push_context();
    entry->as_type().st = ctx().st = st()->make_child(etable_owner::STRUCT);
    ctx()._struct = stype;
    auto cg = guard();
    
    ast* scontent{nullptr};
    
    if (is(Symbol::BRACE_LEFT)) {
         scontent = structscope();
         types.update_type(stype->id, *stype);
         entry->as_type().defined = true;
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
    
    return ast::binary(Symbol::SYMDECL, ast::symbol(entry), scontent ? scontent : ast::none(), stype);
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
    next(); // }
    
    return block;
}

ast* parser::uniondecl() {
    compiler_assert(Keyword::UNION);
    next(); // union
    
    require(TokenType::IDENTIFIER);
    
    std::string name = c.value;
    next(); // iden
    
    require(Symbol::BRACE_LEFT);
    
    bool declared{false};
    
    type* utype{nullptr};
    
    st_entry* entry = st()->get(name, false);
    if (entry) {
        if (!entry->is_type() || entry->as_type().defined || !entry->as_type().t->is_union()) {
            error("Union already exists", epanic_mode::ESCAPE_BRACE);
            next(); // }
            return nullptr;
        }
        declared = true;
        utype = entry->as_type().t;
    } else {
        utype = types.add_type(type::_union());
        entry = st()->add_type(name, utype, false);
        utype->as_union().ste = entry;
    }
    
    symbol_table* parent = st();
    push_context();
    entry->as_type().st = ctx().st = st()->make_child(etable_owner::UNION);
    ctx()._struct = utype;
    auto cg = guard();
    
    ast* ucontent{nullptr};
    
    if (is(Symbol::BRACE_LEFT)) {
         ucontent = unionscope();
         types.update_type(utype->id, *utype);
         entry->as_type().defined = true;
    } else {
        if (declared) {
            require(Symbol::SEMICOLON);
            next(); // ;
            return error("Union already declared");
        }
        require(Symbol::SEMICOLON);
        next(); // ;
    }

    cg.deactivate();
    
    return ast::binary(Symbol::SYMDECL, ast::symbol(entry), ucontent, utype);
}

ast* parser::unionscope() {
    compiler_assert(Symbol::BRACE_LEFT);
    next(); // {
    
    ast* block = ast::block(st());
    auto& stmts = block->as_block().stmts;
    
    while (!is(Symbol::BRACE_RIGHT) && !is(TokenType::END_OF_FILE)) {
        stmts.push_back(uniondeclstmt());
    }
    
    require(Symbol::BRACE_RIGHT);
    next(); // }
    
    return block;
}

ast* parser::enumdecl() {
    compiler_assert(Keyword::ENUM);
    next(); // enum
    
    require(TokenType::IDENTIFIER);
    
    std::string name = c.value;
    next(); // iden
    
    require(Symbol::BRACE_LEFT);
    
    bool declared{false};
    
    type* etype {nullptr};
    
    st_entry* entry = st()->get(name, false);
    if (entry) {
        if (!entry->is_type() || entry->as_type().defined || !entry->as_type().t->is_enum()) {
            error("Enum already exists", epanic_mode::ESCAPE_BRACE);
            next(); // }
            return nullptr;
        }
        declared = true;
        etype = entry->as_type().t;
    } else {
        etype = types.add_type(type::_enum());
        entry = st()->add_type(name, etype, false);
        etype->as_enum().ste = entry;
    }
    
    symbol_table* parent = st();
    push_context();
    entry->as_type().st = ctx().st = st()->make_child(etable_owner::ENUM);
    ctx()._struct = etype;
    auto cg = guard();
    
    ast* econtent{nullptr};
    
    if (is(Symbol::BRACE_LEFT)) {
         econtent = enumscope();
         entry->as_type().defined = true;
    } else {
        if (declared) {
            require(Symbol::SEMICOLON);
            next(); // ;
            return error("Union already declared");
        }
        require(Symbol::SEMICOLON);
        next(); // ;
    }

    cg.deactivate();
    
    return ast::binary(Symbol::SYMDECL, ast::symbol(entry), econtent, etype);
}

ast* parser::enumscope() {
    compiler_assert(Symbol::BRACE_LEFT);
    next(); // {
    
    ast* block = ast::block(st());
    auto& stmts = block->as_block().stmts;
    
    std::set<std::string> values{};
    
    while (!is(Symbol::BRACE_RIGHT) && !is(TokenType::END_OF_FILE)) {
        require(TokenType::IDENTIFIER);
        std::string name = c.value;
        next();
        
        st_entry* added{nullptr};
        
        if (values.find(name) != values.end()) {
            error("Name already exists");
        } else {
            values.insert(name);
            added = st()->add_field(name, st()->get_size(), ctx()._struct);
        }
        
        stmts.push_back(ast::binary(Symbol::SYMDECL, ast::symbol(added), ast::qword(st()->get_size() - 1, ctx()._struct), ctx()._struct));
        
        if (is(Symbol::COMMA)) {
            next(); // ,
        } else {
            require(Symbol::BRACE_RIGHT, epanic_mode::ESCAPE_BRACE);
        }
    }
    require(Symbol::BRACE_RIGHT);
    next(); // }
    
    return block;
}

ast* parser::assstmt() {
    ast* ret = assignment();
    require(Symbol::SEMICOLON);
    next(); // ;
    return ret;
}

ast* parser::assignment() {
    ast* lhs = ast::block(st());
    auto& lstmts = lhs->as_block().stmts;
    
    ast* rhs = ast::block(st());
    auto& rstmts = rhs->as_block().stmts;
    
    bool first{true};
    bool done{false};
    
    std::vector<type*> lhtypes{};
    
    do {
        if (first) {
            first = false;
        } else {
            next(); // ,
        }
        
        ast* exp = expression();
        if (!exp->is_assignable()) {
            error("Cannot assign to expression");
            continue;
        } else {
            lstmts.push_back(exp);
        }
        
        type* t = exp->get_type();
        if (t->is_combination()) {
            auto& comb = t->as_combination().types;
            lhtypes.insert(lhtypes.end(), comb.begin(), comb.end());
        } else {
            lhtypes.push_back(t);
        }
    } while (is(Symbol::COMMA));
    
    Symbol ass;
    if (!is_assignment_operator()) {
        require(Symbol::ASSIGN);
        ass = Symbol::ASSIGN;
    } else {
        ass = c.as_symbol();
    }
    
    next(); // =
    
    u64 i{0};
    
    first = true;
    
    do {
        if (first) {
            first = false;
        } else {
            next(); // ,
        }
        
        push_context();
        if (i >= lhtypes.size()) {
            // error("Too many assignments", epanic_mode::SEMICOLON);
            done = true;
            ctx().expected = types.t_let;
        } else {
            ctx().expected = lhtypes[i];
        }
        auto cg = guard();
        
        ast* aexp = aexpression();
        
        if (done) {
            continue;
        }
        
        rstmts.push_back(aexp);
        
        type* t = aexp->get_type();
        if (t->is_combination()) {
            auto& comb = t->as_combination().types;
            for (type* ct : comb) {
                type* restype = get_result_type(ass, lhtypes[i++], t);
                if (!restype) {
                    operator_error(ass, lhtypes[i - 1], t);
                    continue;
                }
                
                if (i >= lhtypes.size()) {
                    done = true;
                    break;
                }
            }
        } else {
            type* restype = get_result_type(ass, lhtypes[i++], t);
            if (!restype) {
                operator_error(ass, lhtypes[i - 1], t);
                continue;
            }
        }
        
    } while (is(Symbol::COMMA));
    
    return ast::binary(ass, lhs, rhs);
}

ast* parser::newinit() {
    compiler_assert(Keyword::NEW);
    
    ast* nntypes = ast::block(st());
    auto& typstmts = nntypes->as_block().stmts;
    ast* values = ast::block(st());
    auto& valstmts = values->as_block().stmts;
    
    std::vector<type*> rettypes{};
    
    do {
        next(); // new ,
    
        bool array{false};
        
        ast* typ = nntype(&array);
        auto& nntyp = typ->as_nntype();
        typstmts.push_back(typ);
        
        bool sized{true};
        
        if (array && !nntyp.array_sizes.empty() && nntyp.array_sizes.back()->get_type() == types.t_void) {
            sized = false;
        }
        
        if (array && !sized) {
            require(Symbol::BRACE_LEFT);
        }
        
        ast* value{nullptr};
        
        if (is(Symbol::BRACE_LEFT)) {
            push_context();
            ctx().expected = nntyp.t;
            auto cg = guard();
            
            next(); // {
            
            value = aexpression();
            if (!value->get_type()->can_weak_cast(nntyp.t)) {
                error("Cannot cast new value to type");
            }
            
            if(require(Symbol::BRACE_RIGHT, epanic_mode::ESCAPE_BRACE)) {
                next(); // }
            }
        } else {
            value = ast::byte(0, nntyp.t);
        } 
        valstmts.push_back(value);
        
        if (!array) {
            type ret{ettype::POINTER};
            auto& ptr = ret.as_pointer();
            ptr.ptr_t = eptr_type::NAKED;
            ptr.t = nntyp.t;
            rettypes.push_back(types.get_or_add(ret));
        } else {
            rettypes.push_back(nntyp.t);
        }
        
    } while (is(Symbol::COMMA));

    type* newtype{nullptr};
    
    if (rettypes.size() > 1) {
        type t{ettype::COMBINATION};
        auto& comb = t.as_combination();
        
        comb.types = rettypes;
        newtype = types.get_or_add(t);
    } else {
        newtype = rettypes.back();
    }
    
    return ast::binary(Symbol::KWNEW, nntypes, values, newtype);
}

ast* parser::deletestmt() {
    compiler_assert(Keyword::DELETE);
    
    ast* deletes = ast::block(st());
    auto& stmts = deletes->as_block().stmts;
    
    do {
        next(); // delete ,
        ast* exp = expression();
        if (!exp->is_assignable()) {
            error("Cannot delete result of expression");
            continue;
        } else if (!exp->get_type()->is_pointer(eptr_type::NAKED) && 
                   !exp->get_type()->is_pointer(eptr_type::ARRAY)) {
            error("Cannot delete smart pointer");
            continue;
        }
        
        stmts.push_back(exp);
        
    } while (is(Symbol::COMMA));
    
    require(Symbol::SEMICOLON, epanic_mode::SEMICOLON);
    next();
    
    return ast::unary(Symbol::KWDELETE, deletes);
}

ast* parser::expressionstmt() {
    ast* exp = expression();
    require(Symbol::SEMICOLON);
    next(); // ;
    
    return exp;
}

ast* parser::expression() {
    return e17();
}

ast* parser::e17() {
    ast* exp = e16();
    if (is(Symbol::TERNARY_CONDITION)) {
        next();
        ast* yes = e17();
        require(Symbol::TERNARY_CHOICE);
        next(); // :
        ast* no = e17();
        if (!exp->get_type()->can_boolean()) {
            std::stringstream ss{};
            ss << "Ternary condition of type \"" << exp->get_type()->print(true) << "\" cannot be converted to boolean";
            error(ss.str());
        } else if (!yes->get_type()->is_weak_equalish(no->get_type())) {
            std::stringstream ss{};
            ss << "Types \"" << yes->get_type()->print(true) << "\" and \"" << no->get_type()->print(true) << "\" of ternary condition results are not compatible";
            error(ss.str());
        }
        ast* block = ast::block(st());
        block->as_block().stmts.push_back(yes);
        block->as_block().stmts.push_back(no);
        exp = ast::binary(Symbol::TERNARY_CONDITION, exp, block, yes->get_type(), true);
    }
    return exp;
}

ast* parser::e16() {
    ast* exp = e15();
    while (is(Symbol::LXOR)) {
        next(); // ^^
        ast* to = e15();
        type* restype = get_result_type(Symbol::LXOR, exp->get_type(), to->get_type());
        if (!restype) {
            operator_error(Symbol::LXOR, exp->get_type(), to->get_type());
            restype = types.t_bool;
        }
        exp = ast::binary(Symbol::LXOR, exp, to, restype);        
    }
    return exp;
}

ast* parser::e15() {
    ast* exp = e14();
    while (is(Symbol::LOR)) {
        next(); // ||
        ast* to = e14();
        type* restype = get_result_type(Symbol::LOR, exp->get_type(), to->get_type());
        if (!restype) {
            operator_error(Symbol::LOR, exp->get_type(), to->get_type());
            restype = types.t_bool;
        }
        exp = ast::binary(Symbol::LOR, exp, to, restype);        
    }
    return exp;
}

ast* parser::e14() {
    ast* exp = e13();
    while (is(Symbol::LAND)) {
        next(); // &&
        ast* to = e14();
        type* restype = get_result_type(Symbol::LAND, exp->get_type(), to->get_type());
        if (!restype) {
            operator_error(Symbol::LAND, exp->get_type(), to->get_type());
            restype = types.t_bool;
        }
        exp = ast::binary(Symbol::LAND, exp, to, restype);
    }
    return exp;
}

ast* parser::e13() {
    ast* exp = e12();
    while (is(Symbol::EQUALS) || is(Symbol::NOT_EQUALS)) {
        Symbol sym = c.as_symbol();
        next(); // == !=
        ast* to = e12();
        type* restype = get_result_type(sym, exp->get_type(), to->get_type());
        if (!restype) {
            operator_error(sym, exp->get_type(), to->get_type());
            restype = types.t_bool;
        }
        exp = ast::binary(sym, exp, to, restype);
    }
    return exp;
}

ast* parser::e12() {
    ast* exp = e11();
    while (is(Symbol::LESS) || is(Symbol::GREATER) || is(Symbol::LESS_OR_EQUALS) || is(Symbol::GREATER_OR_EQUALS)) {
        Symbol sym = c.as_symbol();
        next(); // < > <= >=
        ast* to = e11();
        type* restype = get_result_type(sym, exp->get_type(), to->get_type());
        if (!restype) {
            operator_error(sym, exp->get_type(), to->get_type());
            restype = types.t_bool;
        }
        exp = ast::binary(sym, exp, to, restype);
    }
    return exp;
}

ast* parser::e11() {
    ast* exp = e10();
    while (is(Symbol::XOR)) {
        next(); // ^
        ast* to = e10();
        type* restype = get_result_type(Symbol::XOR, exp->get_type(), to->get_type());
        if (!restype) {
            operator_error(Symbol::XOR, exp->get_type(), to->get_type());
            restype = type::weak_cast_result(exp->get_type(), to->get_type());
        }
        exp = ast::binary(Symbol::XOR, exp, to, restype);
    }
    return exp;
}

ast* parser::e10() {
    ast* exp = e9();
    while (is(Symbol::OR)) {
        next(); // |
        ast* to = e9();
        type* restype = get_result_type(Symbol::XOR, exp->get_type(), to->get_type());
        if (!restype) {
            operator_error(Symbol::OR, exp->get_type(), to->get_type());
            restype = type::weak_cast_result(exp->get_type(), to->get_type());
        }
        exp = ast::binary(Symbol::OR, exp, to, restype);        
    }
    return exp;
}

ast* parser::e9() {
    ast* exp = e8();
    while (is(Symbol::AND)) {
        next(); // &
        ast* to = e8();
        type* restype = get_result_type(Symbol::XOR, exp->get_type(), to->get_type());
        if (!restype) {
            operator_error(Symbol::AND, exp->get_type(), to->get_type());
            restype = type::weak_cast_result(exp->get_type(), to->get_type());
        }
        exp = ast::binary(Symbol::AND, exp, to, restype);
    }
    return exp;
}

ast* parser::e8() {
    ast* exp = e7();
    while (is(Symbol::BIT_SET) || is(Symbol::BIT_CLEAR) || is(Symbol::BIT_CHECK) || is(Symbol::BIT_TOGGLE)) {
        Symbol sym = c.as_symbol();
        next(); // @| @& @? @^
        ast* to = e7();
        type* restype = get_result_type(sym, exp->get_type(), to->get_type());
        if (!restype) {
            operator_error(sym, exp->get_type(), to->get_type());
            restype = sym == Symbol::BIT_CHECK ? types.t_bool : exp->get_type();
        }
        exp = ast::binary(sym, exp, to, restype);
    }
    return exp;
}

ast* parser::e7() {
    ast* exp = e6();
    while (is(Symbol::SHIFT_LEFT) || is(Symbol::SHIFT_RIGHT) || is(Symbol::ROTATE_LEFT) || is(Symbol::ROTATE_RIGHT)) {
        Symbol sym = c.as_symbol();
        next(); // << >> <<< >>>
        ast* to = e6();
        type* restype = get_result_type(sym, exp->get_type(), to->get_type());
        if (!restype) {
            operator_error(sym, exp->get_type(), to->get_type());
            restype = exp->get_type();
        }
        exp = ast::binary(sym, exp, to, restype);
    }
    return exp;
}

ast* parser::e6() {
    ast* exp = e5();
    while (is(Symbol::ADD) || is(Symbol::SUBTRACT) || is(Symbol::CONCATENATE)) {
        Symbol sym = c.as_symbol();
        next(); // + - ..
        ast* to = e6();
        type* restype = get_result_type(sym, exp->get_type(), to->get_type());
        if (!restype) {
            operator_error(sym, exp->get_type(), to->get_type());
            restype = sym == Symbol::CONCATENATE ? exp->get_type() : type::weak_cast_result(exp->get_type(), to->get_type());
        }
        exp = ast::binary(sym, exp, to, restype);
    }
    return exp;
}

ast* parser::e5() {
    ast* exp = e4();
    while (is(Symbol::MULTIPLY) || is(Symbol::DIVIDE) || is(Symbol::MODULO)) {
        Symbol sym = c.as_symbol();
        next(); // * / %
        ast* to = e4();
        type* restype = get_result_type(sym, exp->get_type(), to->get_type());
        if (!restype) {
            operator_error(sym, exp->get_type(), to->get_type());
            restype = sym == Symbol::MODULO ? exp->get_type() : type::weak_cast_result(exp->get_type(), to->get_type());
        }
        exp = ast::binary(sym, exp, to, restype);
    }
    return exp;
}

ast* parser::e4() {
    ast* exp = e3();
    if (is(Symbol::POWER)) {
        next();
        ast* to = e4();
        type* restype = get_result_type(Symbol::POWER, exp->get_type(), to->get_type());
        if (!restype) {
            operator_error(Symbol::POWER, exp->get_type(), to->get_type());
            restype = types.t_double;
        }
        exp = ast::binary(Symbol::POWER, exp, to, restype);
    }
    return exp;
}

ast* parser::e3() {
    if (is(Symbol::INCREMENT) || is(Symbol::DECREMENT) || is(Symbol::ADD) || is(Symbol::SUBTRACT) || is(Symbol::LENGTH) || is(Symbol::NOT) || is(Symbol::LNOT) || is(Symbol::AT) || is(Symbol::ADDRESS)) {
        Symbol sym = c.as_symbol();
        next(); // ++ -- + - ~ ! !! @ *
        ast* exp = e3();
        type* etype = get_result_type(sym, exp->get_type());
        if (!etype) {
            operator_error(sym, exp->get_type(), false);
        }
        bool assignable{false};
        switch (sym) {
            case Symbol::INCREMENT: [[fallthrough]];
            case Symbol::DECREMENT: 
                if (!etype) {
                    etype = exp->get_type();
                }
                break;
            case Symbol::ADD: [[fallthrough]];
            case Symbol::SUBTRACT: 
                if (!etype) {
                    etype = exp->get_type();
                }
                break;
            case Symbol::LENGTH:
                if (!etype) {
                    etype = types.t_long;
                }
                break;
            case Symbol::NOT: 
                if (!etype) {
                    etype = exp->get_type();
                }
                break;
            case Symbol::LNOT:
                if (!etype) {
                    etype = types.t_bool;
                }
                break;
            case Symbol::AT:
                if (!etype) {
                    etype = types.t_void;
                } else {
                    assignable = true;
                }
                break;
            case Symbol::ADDRESS: // Cannot go wrong?
                break;
            default:
                error("How did we get here", epanic_mode::ULTRA_PANIC);
                break;
        }
        return ast::unary(sym, exp, etype, assignable, false);
    } else if (is(Symbol::LT)) {
        next(); // <
        ast* typ = nntype();
        require(Symbol::GT);
        next(); // >
        
        push_context();
        ctx().expected = typ->get_type();
        auto cg = guard();
        
        ast* from = e3();
        
        type* etype = get_result_type(Symbol::CAST, typ->get_type(), from->get_type());
        if (!etype) {
            std::stringstream ss{};
            ss << "Cannot cast \"" << from->get_type()->print(true) << "\" to \"" << typ->get_type()->print(true) << "\"";
            error(ss.str());
            etype = typ->get_type();
        }
        
        return ast::binary(Symbol::CAST, from, typ, etype);
    } else {
        return e2();
    }
}

ast* parser::e2() {
    ast* exp = e1();
    while (is(Symbol::SPREAD)) {
        next(); // ...
        if (!exp->get_type()->is_combination() && !exp->get_type()->is_pointer(eptr_type::ARRAY)) {
            operator_error(Symbol::SPREAD, exp->get_type());
        }
        exp = ast::binary(Symbol::SPREAD, exp, exp, exp->get_type());
    }
    return exp;
}

ast* parser::e1() {
    ast* exp = ee();
    while (is(Symbol::INCREMENT) || is(Symbol::DECREMENT) || is(Symbol::PAREN_LEFT) || 
           is(Symbol::BRACKET_LEFT) || is(Symbol::ACCESS)) {
        Symbol sym = c.as_symbol();
        next(); // ++ -- ( [ .
        type* restype{nullptr};
        switch (sym) {
            case Symbol::INCREMENT: [[fallthrough]];
            case Symbol::DECREMENT: 
                restype = get_result_type(sym, exp->get_type());
                if (!restype) {
                    operator_error(sym, exp->get_type());
                    restype = exp->get_type();
                }
                exp = ast::unary(sym, exp, restype);
                break;
            case Symbol::PAREN_LEFT: {
                ast* args = ast::block(st());
                auto& stmts = args->as_block().stmts;
                type* rtype{nullptr};
                bool first_param = ctx().first_param != nullptr; // TODO Convert to pointer type
                if (exp->get_type()->is_function(true)) {
                    auto& params = exp->get_type()->as_pfunction().params;
                    rtype = exp->get_type()->as_pfunction().rets;
                    stmts = std::vector<ast*>(params.size(), nullptr);
                    u64 i{0};
                    while (ctx().first_param != nullptr || (!is(Symbol::PAREN_RIGHT) && !is(TokenType::END_OF_FILE))) {
                        if (i >= params.size()) {
                            error("Too many arguments", epanic_mode::ESCAPE_PAREN);
                            continue;
                        }
                        
                        bool first_param = ctx().first_param != nullptr;
                        
                        push_context();
                        ctx().expected = params[i].t;
                        auto cg = guard();
                        
                        auto [val, name] = argument(false);
                        type* argtype = val->get_type();
                        
                        u64 si = i;
                        
                        if (argtype->is_combination()) {
                            for (type* t : argtype->as_combination().types) {
                                if (i >= params.size()) {
                                    error("Too many arguments", epanic_mode::ESCAPE_PAREN);
                                    continue;
                                }
                                if (!t->can_weak_cast(params[i].t)) {
                                    error("Cannot cast argument to parameter type");
                                }
                                if ((params[i].flags & eparam_flags::SPREAD) == 0) {
                                    ++i;
                                }
                            }
                        } else {
                            if (!argtype->can_weak_cast(params[i].t)) {
                                error("Cannot cast argument to parameter type");
                            }
                            if ((params[i].flags & eparam_flags::SPREAD) == 0) {
                                ++i;
                            }
                        }
                        
                        stmts[si] = val;
                        
                        if (first_param) {
                            cg.deactivate();
                            ctx().first_param = nullptr;
                        } else if (is(Symbol::COMMA)) {
                            next(); // ,
                        } else {
                            require(Symbol::PAREN_RIGHT);
                        }
                    }
                    
                    if (i < params.size() && !(params[i].flags & eparam_flags::DEFAULTABLE) && !(params[i].flags & eparam_flags::SPREAD)) {
                        error("Not enough arguments");
                    } 
                    
                } else if (exp->get_type()->is_function(false)) {
                    auto& params = exp->get_type()->as_function().params; // TODO Revise
                    rtype = exp->get_type()->as_function().rets;
                    stmts = std::vector<ast*>(params.size(), nullptr);
                    std::vector<bool> passed(params.size(), false);
                    u64 i{0}, maxi{0};
                    bool named{false};
                    while (ctx().first_param != nullptr || (!is(Symbol::PAREN_RIGHT) && !is(TokenType::END_OF_FILE))) {
                        if (i >= params.size()) {
                            error("Too many arguments", epanic_mode::ESCAPE_PAREN);
                            continue;
                        }
                        
                        bool first_param = ctx().first_param != nullptr;
                        
                        push_context();
                        ctx().expected = params[i].t;
                        auto cg = guard();
                        
                        auto [val, name] = argument(true, exp->get_type());
                        type* argtype = val->get_type();
                        
                        u64 si = i;
                        
                        if (name.empty() && !named) {
                            if (argtype->is_combination()) {
                                for (type* t : argtype->as_combination().types) {
                                    if (i >= params.size()) {
                                        error("Too many arguments", epanic_mode::ESCAPE_PAREN);
                                        continue;
                                    }
                                    if (!t->can_weak_cast(params[i].t)) {
                                        error("Cannot cast argument to parameter type");
                                    }
                                    passed[i] = true;
                                    if ((params[i].flags & eparam_flags::SPREAD) == 0) {
                                        ++i;
                                    }
                                }
                            } else {
                                if (!argtype->can_weak_cast(params[i].t)) {
                                    error("Cannot cast argument to parameter type");
                                }
                                passed[i] = true;
                                if ((params[i].flags & eparam_flags::SPREAD) == 0) {
                                    ++i;
                                }
                            }
                            maxi = i;
                        } else {
                            if (name.empty()) {
                                error("Cannot have non-named parameters after named parameters");
                                continue;
                            }
                            
                            for (i = 0; i < params.size(); ++i) {
                                if (params[i].name == name) {
                                    break;
                                }
                            } 
                            si = i;
                            if (i >= params.size()) {
                                error("No parameter with such name");
                                continue;
                            }
                            if (argtype->is_combination()) {
                                error("Cannot pass combination type to a single parameter");
                            } else {
                                if (passed[i]) {
                                    error("Parameter already filled in");
                                    continue;
                                }
                                if (params[i].flags & eparam_flags::SPREAD) {
                                    if (!argtype->is_pointer(eptr_type::ARRAY) || !argtype->as_pointer().t->can_weak_cast(params[i].t)) {
                                        error("Cannot cast argument to parameter type");
                                    }
                                } else {
                                    if (!argtype->can_weak_cast(params[i].t)) {
                                        error("Cannot cast argument to parameter type");
                                    }
                                }
                                passed[i] = true;
                            }
                        }
                        
                        stmts[si] = val;
                        
                        if (first_param) {
                            cg.deactivate();
                            ctx().first_param = nullptr;
                        } else if (is(Symbol::COMMA)) {
                            next(); // ,
                        } else {
                            require(Symbol::PAREN_RIGHT);
                        }
                    }
                    
                    for (auto& stmt : stmts) {
                        if (!stmt) {
                            stmt = ast::byte(0, types.t_nothing);
                        }
                    }
                    
                    u64 last_passed = 0;
                    for (; last_passed < passed.size(); ++last_passed) {
                        if (!passed[last_passed]) {
                            break;
                        }
                    }
                    
                    if (last_passed < params.size() && !(params[last_passed].flags & eparam_flags::DEFAULTABLE) && !(params[last_passed].flags & eparam_flags::SPREAD)) {
                        error("Not enough arguments");
                    } 
                } else if (exp->is_symbol() && exp->get_type() == types.t_fun) {
                    stmts = std::vector<ast*>{};
                    std::vector<type*> argtypes{};
                    while (ctx().first_param != nullptr || (!is(Symbol::PAREN_RIGHT) && !is(TokenType::END_OF_FILE))) {
                        bool first_param = ctx().first_param != nullptr;
                        
                        push_context();
                        auto cg = guard();
                        
                        auto [val, name] = argument(false);
                        type* argtype = val->get_type();
                        
                        if (argtype->is_combination()) {
                            for (type* t : argtype->as_combination().types) {
                                argtypes.push_back(t);
                            }
                        } else {
                            argtypes.push_back(argtype);
                        }
                        
                        stmts.push_back(val);
                        
                        if (first_param) {
                            cg.deactivate();
                            ctx().first_param = nullptr;
                        } else if (is(Symbol::COMMA)) {
                            next(); // ,
                        } else {
                            require(Symbol::PAREN_RIGHT);
                        }
                    }
                    overload* ol = exp->as_symbol().symbol->as_function().get_overload(argtypes);
                    if (!ol) {
                        error("Cannot find overload taking those parameters");
                        next(); // )
                        rtype = types.t_void;
                        continue;
                    } else {
                        rtype = ol->t->as_function().rets;
                        exp->as_symbol().overload = ol->oid;
                        exp->as_symbol().overload_defined = true;
                    }
                } else {
                    error("Cannot call non-function", epanic_mode::ESCAPE_PAREN);
                    next(); // )
                    rtype = types.t_void;
                    continue;
                }
                
                require(Symbol::PAREN_RIGHT, epanic_mode::ESCAPE_PAREN);
                next(); // )
                
                exp = ast::binary(Symbol::FUN_CALL, exp, args, rtype);
                break;
            }
            case Symbol::BRACKET_LEFT: {
                ast* to = expression();
                require(Symbol::BRACKET_RIGHT, epanic_mode::ESCAPE_BRACKET);
                next(); // ]
                type* restype = get_result_type(Symbol::INDEX, exp->get_type(), to->get_type());
                if (!restype) {
                    operator_error(Symbol::INDEX, exp->get_type(), to->get_type());
                    restype = exp->get_type()->is_pointer(eptr_type::ARRAY) ? exp->get_type()->as_pointer().t : types.t_void;
                }
                exp = ast::binary(Symbol::INDEX, exp, to, restype, true);
                break;
            }
            case Symbol::ACCESS: {
                bool not_first = exp->is_binary() && exp->as_binary().op == Symbol::ACCESS;
                ast* on = not_first ? exp->as_binary().right : exp;
                require(TokenType::IDENTIFIER);
                std::string name = c.value;
                next(); // iden
                type* t{nullptr};
                if (on->is_symbol()) {
                    st_entry* symbol = on->as_symbol().symbol;
                    switch (symbol->t) {
                        case est_entry_type::TYPE: {
                            st_type& t = symbol->as_type();
                            if (t.t->is_struct() || t.t->is_union()) {
                                st_entry* meth = t.st->get(name, est_entry_type::FUNCTION, false);
                                if (!meth) {
                                    error("No method with that name");
                                } else {
                                    if (is(Symbol::PAREN_LEFT)) {
                                        ctx().first_param = exp;
                                    } else {
                                        delete exp; // Bwuh?
                                    }
                                    exp = ast::symbol(meth);
                                    continue;
                                }
                            } else if (t.t->is_enum()) {
                                st_entry* en = t.st->get(name, est_entry_type::FIELD, false);
                                if (!en) {
                                    error("No enumeration value with that name");
                                } else {
                                    delete exp;
                                    exp = ast::qword(en->as_field().field, t.t);
                                    continue;
                                }
                            } else {
                                return error("Impossible! The ancient scripts must be mistaken!", epanic_mode::ULTRA_PANIC);
                            }
                            break;
                        }
                        case est_entry_type::VARIABLE:
                            t = symbol->get_type();
                            break;
                        case est_entry_type::FUNCTION: {
                            st_function& f = symbol->as_function();
                            st_entry* sig = f.st->get(name, est_entry_type::FIELD, false);
                            if (sig) {
                                delete exp;
                                exp = ast::qword(sig->as_field().field, types.t_sig);
                                continue;
                            } else {
                                t = symbol->get_type();
                            }
                            break;
                        }
                        case est_entry_type::MODULE: [[fallthrough]];
                        case est_entry_type::NAMESPACE: {
                            st_entry* e = symbol->as_namespace().st->get(name, false);
                            if (e) {
                                delete exp;
                                exp = ast::symbol(e);
                                continue;
                            } else {
                                error("No such name in namespace");
                            }
                            break;
                        }
                        case est_entry_type::FIELD:
                            t = symbol->get_type();
                            if (!not_first) {
                                st_entry* zhis = st()->get("this");
                                if (zhis->get_type()->as_pointer().t != symbol->as_field().ptype) {
                                    error("this does not have any such fields");
                                } else {
                                    exp = ast::binary(Symbol::ACCESS, ast::symbol(zhis), on, t, true);
                                }
                            }
                            break;
                        case est_entry_type::LABEL:
                            error("I thought colons could not be part of names...", epanic_mode::ULTRA_PANIC);
                            break;
                    }
                } else {
                    t = on->get_type();
                }
                
                if (t) {
                    for (int i = 2; i; i--) { // Run twice
                        if (t->is_struct() || t->is_union()) {
                            symbol_table* find = t->is_struct() ? t->as_struct().ste->as_type().st : t->as_union().ste->as_type().st;
                            st_entry* e = find->get(name, false);
                            if (e) {
                                if (e->is_field()) {
                                    exp = ast::binary(Symbol::ACCESS, exp, ast::symbol(e), e->get_type(), true);
                                    goto access_type_fine;
                                } else if (e->is_function()) {
                                    if (is(Symbol::PAREN_LEFT)) {
                                        ctx().first_param = ast::unary(Symbol::ADDRESS, exp, pointer_to(t));
                                        exp = ast::symbol(e);
                                        goto access_type_fine;
                                    } else {
                                        error("Method access on values or variables must be used as function calls");
                                    }
                                }
                            }
                        }
                        st_entry* ff = ctx().st->get(name, est_entry_type::FUNCTION);
                        if (ff && ff->is_function()) {
                            if (is(Symbol::PAREN_LEFT)) {
                                ctx().first_param = exp;
                                exp = ast::symbol(ff);
                                goto access_type_fine;
                            } else {
                                error("Function access on values or variables must be used as function calls");
                            }
                        }
                        if (t && t->is_pointer()) {
                            t = t->as_pointer().t;
                        } else {
                            break;
                        }
                    }
                    error("Could not find any names");
                    access_type_fine:;
                } else {
                    error("Should have already returned, fool!", epanic_mode::ULTRA_PANIC);
                }
                break;
            }
            default:
                error("Oh no", epanic_mode::ULTRA_PANIC);
                break;
        }
    }
    return exp;
}

ast* parser::ee() {
    ast* ret{nullptr};
    
    if (is(Symbol::PAREN_LEFT)) {
        next(); // (
        if (is_type() || is_infer()) {
            ret = funcval();
        } else {
            ret = expression();
        }
        require(Symbol::PAREN_RIGHT, epanic_mode::ESCAPE_PAREN);
        next(); // )
    } else if (ctx().expected && (is(Symbol::BRACE_LEFT) || is(Symbol::BRACKET_LEFT))) {
        ret = compound_literal();
        ctx().expected = nullptr;
    } else if (is(TokenType::IDENTIFIER)) {
        ret = iden(true);
    } else {
        ret = safe_literal();
    }
    return ret;
}

std::pair<ast*, std::string> parser::argument(bool allow_names, type* ftype) {
    if (ctx().first_param) {
        ast* buff = ctx().first_param;
        ctx().first_param = nullptr;
        return {buff, {}};
    }
    if (allow_names && peek(Symbol::ASSIGN)) {
        require(TokenType::IDENTIFIER);
        std::string paramname = c.value;
        next(); // iden
        next(); // =
        if (ftype) {
            auto& params = ftype->as_function().params;
            type* etype{nullptr};
            for (auto& param : params) {
                if (param.name == paramname) {
                    etype = param.t;
                    break;
                }
            }
            ctx().expected = etype; // Cleared outside
        }
        return {aexpression(), paramname};
    } else {
        return {aexpression(), {}};
    }
    return {nullptr, {}};
}

ast* parser::fexpression(eexpression_type* expression_type) {
    ast* ret{nullptr};
    if (is_type() || is_infer()) {
        ret = freevardecl();
        if (expression_type) {
            *expression_type = eexpression_type::DECLARATION;
        }
    } else {
        ret = mexpression(expression_type);
    }
    return ret;
}

ast* parser::mexpression(eexpression_type* expression_type) {
    ast* ret{nullptr};
    u64 amount = peek_until({
        Symbol::ASSIGN, 
        Symbol::ADD_ASSIGN, 
        Symbol::SUBTRACT_ASSIGN, 
        Symbol::MULTIPLY_ASSIGN, 
        Symbol::POWER_ASSIGN, 
        Symbol::DIVIDE_ASSIGN, 
        Symbol::AND_ASSIGN, 
        Symbol::OR_ASSIGN, 
        Symbol::XOR_ASSIGN, 
        Symbol::SHIFT_LEFT_ASSIGN, 
        Symbol::SHIFT_RIGHT_ASSIGN, 
        Symbol::ROTATE_LEFT_ASSIGN, 
        Symbol::ROTATE_RIGHT_ASSIGN, 
        Symbol::CONCATENATE_ASSIGN, 
        Symbol::BIT_SET_ASSIGN, 
        Symbol::BIT_CLEAR_ASSIGN, 
        Symbol::BIT_TOGGLE_ASSIGN, 
        Symbol::SEMICOLON, 
        Symbol::BRACE_LEFT
    }, true);
    token t = l->peek(amount);
    if (is_assignment_operator(t.as_symbol())) {
        ret = assignment();
        if (expression_type) {
            *expression_type = eexpression_type::ASSIGNMENT;
        }
    } else {
        ret = expression();
        if (expression_type) {
            *expression_type = eexpression_type::EXPRESSION;
        }
    }
    return ret;
}

ast* parser::aexpression() {
    if (is(Keyword::NEW)) {
        return newinit();
    } else if (is(Symbol::BRACE_LEFT) || is(Symbol::BRACKET_LEFT)) {
        return compound_literal();
    } else if (is(Symbol::NOTHING)) {
        next(); // ---
        return ast::byte(0, types.t_nothing);
    } else {
        return expression();
    }
}

bool parser::is_type() {
    if (c.type == TokenType::IDENTIFIER) {
        auto& tok = c;
        bool ret = false;
        u64 pos = 0;
        symbol_table* cst{st()};
        do {
            auto entry = cst->get(c.value);
            ret = entry && entry->is_type();
            cst = ret ? entry->as_type().st : nullptr;
            pos += 2;
        } while (ret && peek(Symbol::ACCESS, pos - 2));
        return ret;
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

bool parser::is_assignment_operator(Symbol sym) {
    Symbol swtch = sym != Symbol::SYMBOL_INVALID ? sym : c.type == TokenType::SYMBOL ? c.as_symbol() : Symbol::SYMBOL_INVALID;
    
    switch (swtch) {
        case Symbol::ASSIGN: [[fallthrough]];
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

type* parser::pointer_to(type* to, eptr_type ptype, u64 size) {
    type np = type{ettype::POINTER};
    np.as_pointer().t = to;
    np.as_pointer().ptr_t = ptype;
    np.as_pointer().size = size;
    return types.get_or_add(np);
}

type* parser::get_result_type(Symbol op, type* t) {
    switch (op) {
        case Symbol::INCREMENT: [[fallthrough]];
        case Symbol::DECREMENT: 
            if (t->is_integer() || t->is_pointer()) {
                return t;
            } else {
                return nullptr;
            }
        case Symbol::ADD:
            if (t->is_numeric()) {
                return t;
            } else {
                return nullptr;
            }
        case Symbol::SUBTRACT: 
            if (t->is_numeric()) {
                return t;
            } else {
                return nullptr;
            }
        case Symbol::LENGTH:
            if (t->is_primitive(etype_ids::STRING) || t->is_pointer(eptr_type::ARRAY)) {
                return types.t_long;
            } else {
                return nullptr;
            }
        case Symbol::NOT:
            if (t->is_integer()) {
                return t;
            } else if (t->is_primitive(etype_ids::BOOL)) {
                return types.t_bool;
            } else {
                return nullptr;
            }
        case Symbol::LNOT:
            if (t->can_boolean()) {
                return types.t_bool;
            } else {
                return nullptr;
            }
        case Symbol::AT:
            if (t->is_pointer()) {
                return t->as_pointer().t;
            } else {
                return nullptr;
            }
        case Symbol::POINTER:
            return pointer_to(t);
        default:
            return nullptr;
    }
}

type* parser::get_result_type(Symbol op, type* l, type* r) {
    switch (op) {
        case Symbol::FUN_CALL:
            return nullptr;
        case Symbol::INDEX:
            if (l->is_pointer(eptr_type::ARRAY) && r->is_integer()) {
                return l->as_pointer().t;
            } else if (l->is_primitive(etype_ids::STRING) && r->is_integer()) {
                return types.t_char;
            } else {
                return nullptr;
            }
        case Symbol::CAST:
            if (r->can_cast(l)) {
                return l;
            } else {
                return nullptr;
            }
        case Symbol::POWER:
            if (l->is_numeric() && r->is_numeric()) {
                return types.t_double;
            } else {
                return nullptr;
            }
        case Symbol::MULTIPLY: [[fallthrough]];
        case Symbol::DIVIDE:
            if (l->is_numeric() && r->is_numeric()) {
                return type::weak_cast_result(l, r);
            } else {
                return nullptr;
            }
        case Symbol::MODULO:
            if (l->is_numeric() && r->is_integer()) {
                return l;
            } else {
                return nullptr;
            }
        case Symbol::ADD: [[fallthrough]];
        case Symbol::SUBTRACT:
            if (l->is_numeric() && r->is_numeric()) {
                return type::weak_cast_result(l, r);
            } else if ((l->is_pointer() ^ r->is_pointer()) && (l->is_numeric() ^ r->is_numeric())) {
                return l->is_pointer() ? l : r;
            } else {
                return nullptr;
            }
        case Symbol::CONCATENATE:
            if (l->is_primitive(etype_ids::STRING)) {
                if (r->is_primitive(etype_ids::STRING) || r->is_primitive(etype_ids::CHAR)) {
                    return l;
                } else {
                    return nullptr;
                }
            } else if (l->is_pointer(eptr_type::ARRAY)) {
                type* lptr = l->as_pointer().t;
                if (r->can_weak_cast(l) || r->can_weak_cast(lptr)) {
                    return l;
                } else {
                    return nullptr;
                }
            } else {
                return nullptr;
            }
        case Symbol::SHIFT_LEFT: [[fallthrough]];
        case Symbol::SHIFT_RIGHT: [[fallthrough]];
        case Symbol::ROTATE_LEFT: [[fallthrough]];
        case Symbol::ROTATE_RIGHT:
            if (l->is_integer() && r->is_integer()) {
                return l;
            } else {
                return nullptr;
            }
        case Symbol::BIT_SET: [[fallthrough]];
        case Symbol::BIT_CLEAR: [[fallthrough]];
        case Symbol::BIT_TOGGLE:
            if (l->is_integer() && r->is_integer()) {
                return l;
            } else {
                return nullptr;
            }
        case Symbol::BIT_CHECK:
            if (l->is_integer() && r->is_integer()) {
                return types.t_bool;
            } else {
                return nullptr;
            }
        case Symbol::AND: [[fallthrough]];
        case Symbol::OR: [[fallthrough]];
        case Symbol::XOR:
            if ((l->is_integer() && r->is_integer()) || (l->is_primitive(etype_ids::BOOL) && r->is_primitive(etype_ids::BOOL))) {
                return l;
            } else {
                return nullptr;
            }
        case Symbol::LESS: [[fallthrough]];
        case Symbol::GREATER: [[fallthrough]];
        case Symbol::LESS_OR_EQUALS: [[fallthrough]];
        case Symbol::GREATER_OR_EQUALS: 
            if ((l->is_numeric() && r->is_numeric()) || 
                (l->is_primitive(etype_ids::CHAR) && r->is_primitive(etype_ids::CHAR)) || 
                (l->is_primitive(etype_ids::STRING) && r->is_primitive(etype_ids::STRING))) {
                return types.t_bool;
            } else {
                return nullptr;
            }
        case Symbol::EQUALS: [[fallthrough]];
        case Symbol::NOT_EQUALS:
            if (type::weak_cast_result(l, r)) {
                return types.t_bool;
            } else {
                return nullptr;
            }
        case Symbol::LAND: [[fallthrough]];
        case Symbol::LOR: [[fallthrough]];
        case Symbol::LXOR:
            if (l->can_boolean() && r->can_boolean()) {
                return types.t_bool;
            } else {
                return nullptr;
            }
        case Symbol::ASSIGN: 
            if (r->can_weak_cast(l) && ((l->flags & etype_flags::CONST) == 0)) {
                return l;
            } else {
                return nullptr;
            }
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
            if (type* t = get_result_type(without_assign(op), l, r)) {
                return get_result_type(Symbol::ASSIGN, l, t);
            } else {
                return nullptr;
            }
        default:
            return nullptr;
    }
}

