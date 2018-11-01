#include "frontend/parser.h"

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

ast* parser::error(const std::string& msg, token* t, bool panic, epanic_mode mode) {
    errors.emplace_back(t ? *t : c, msg);
    if (panic) {
        parser::panic(mode);
    }
    return nullptr; // In case we want to make an error AST or stub
}

void parser::panic(epanic_mode mode) {
    switch(mode) {
        case epanic_mode::SEMICOLON: {
            u64 i = 0;
            while (l->peek(i).as_symbol() != Symbol::SEMICOLON && l->peek(i).type != TokenType::END_OF_FILE) {
                ++i;
            }
            l->skip(i);
            next(); // Into the semicolon
        }
        break;
        case epanic_mode::COMMA: {
            u64 i = 0;
            while (l->peek(i).as_symbol() != Symbol::COMMA && l->peek(i).type != TokenType::END_OF_FILE) {
                ++i;
            }
            l->skip(i);
            next(); // Into the comma
        }
        break;
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

bool parser::require(Grammar::TokenType tt, const std::string& err) {
    if (!is(tt)) {
        if (err.empty()) {
            std::stringstream ss{};
            ss << c.get_info() << " - Expected \"" << Grammar::tokentype_names.at(tt) << " but got \"" << c.value << "\" instead,";
            error(ss.str());
        } else {
            error(err);
        }
        return false;
    }
    return true;
}

bool parser::require(Grammar::Symbol sym, const std::string& err) {
    if (!is(sym)) {
        if (err.empty()) {
            std::stringstream ss{};
            ss << c.get_info() << " - Expected symbol \"" << Grammar::symbol_names.at(sym) << " but got \"" << c.value << "\" instead";
            error(ss.str());
        } else {
            error(err);
        }
        return false;
    }
    return true;
}

bool parser::require(Grammar::Keyword kw, const std::string& err) {
    if (!is(kw)) {
        if (err.empty()) {
            std::stringstream ss{};
            ss << c.get_info() << " - Expected keyword \"" << Grammar::keyword_names.at(kw) << " but got \"" << c.value << "\" instead";
            error(ss.str());
        } else {
            error(err);
        }
        return false;
    }
    return true;
}

ast* parser::iden() {
    if (is(Keyword::THIS)) {
        require(TokenType::IDENTIFIER);
    }
    
    auto tok = next(); // iden
    auto sym = ctx().st->get(tok.value);
    if (!sym) {
        std::stringstream ss{};
        ss << '"' << tok.value << '"' << "does not exist";
        return error(ss.str(), &tok);
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
    require(TokenType::NUMBER);
    return ast::qword(next().as_integer(), types.t_long); // TODO Separate
}

ast* parser::string() {
    require(TokenType::STRING);
    std::string str = next().as_string(); // String
    u8* chars = new u8[str.length()];
    std::memcpy(chars, str.data(), str.length());
    return ast::string(chars, utils::utflen(str));
}

ast* parser::character() {
    require(TokenType::CHARACTER);
    u8 len = c.value.length();
    u32 ch = 0;
    std::memcpy(&ch, next().value.data(), len); // Char
    return ast::dword(ch, types.t_char);
}

ast* parser::array() {
    require(Symbol::BRACKET_LEFT);
    std::vector<ast*> elems{};
    type* t = types.t_let;
    
    do {
        next(); // [ ,
        if (is(Symbol::BRACKET_RIGHT)) {
            break;
        } else {
            ast* exp = expression();
            if (!exp->get_type()->can_weak_cast(t)) {
                return error("Cannot cast to array type", nullptr, true, epanic_mode::COMMA);
            }
        }
    } while (is(Symbol::COMMA));
}

ast* parser::struct_lit() {
    return nullptr;
}

ast* parser::literal() {
    return nullptr;
}

ast* parser::program() {
    return nullptr;
}

ast* parser::statement() {
    return nullptr;
}

ast* parser::scopestatement() {
    return nullptr;
}

ast* parser::scope() {
    return nullptr;
}

ast* parser::ifstmt() {
    return nullptr;
}

ast* parser::ifscope() {
    return nullptr;
}

ast* parser::forstmt() {
    return nullptr;
}

ast* parser::forcond() {
    return nullptr;
}

ast* parser::whilestmt() {
    return nullptr;
}

ast* parser::switchstmt() {
    return nullptr;
}

ast* parser::switchscope() {
    return nullptr;
}

ast* parser::casestmt() {
    return nullptr;
}

ast* parser::trystmt() {
    return nullptr;
}

ast* parser::returnstmt() {
    return nullptr;
}

ast* parser::raisestmt() {
    return nullptr;
}

ast* parser::gotostmt() {
    return nullptr;
}

ast* parser::labelstmt() {
    return nullptr;
}

ast* parser::deferstmt() {
    return nullptr;
}

ast* parser::breakstmt() {
    return nullptr;
}

ast* parser::continuestmt() {
    return nullptr;
}

ast* parser::leavestmt() {
    return nullptr;
}

ast* parser::importstmt() {
    return nullptr;
}

ast* parser::usingstmt() {
    return nullptr;
}

ast* parser::namespacestmt() {
    return nullptr;
}

ast* parser::namespacescope() {
    return nullptr;
}

ast* parser::typemod() {
    return nullptr;
}

ast* parser::nntype() {
    return nullptr;
}

ast* parser::infer() {
    return nullptr;
}

ast* parser::freedeclstmt() {
    return nullptr;
}

ast* parser::structdeclstmt() {
    return nullptr;
}

ast* parser::vardeclass() {
    return nullptr;
}

ast* parser::freevardecliden() {
    return nullptr;
}

ast* parser::freevardecl() {
    return nullptr;
}

ast* parser::structvardecliden() {
    return nullptr;
}

ast* parser::structvardecl() {
    return nullptr;
}

ast* parser::funcdecl() {
    return nullptr;
}

ast* parser::funcval() {
    return nullptr;
}

ast* parser::nnparameter() {
    return nullptr;
}

ast* parser::structdecl() {
    return nullptr;
}

ast* parser::structscope() {
    return nullptr;
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

ast* parser::expressionstmt() {
    return nullptr;
}

ast* parser::expression() {
    return nullptr;
}

ast* parser::newexpr() {
    return nullptr;
}

ast* parser::deleteexpr() {
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

ast* parser::fexpression() {
    return nullptr;
}

ast* parser::mexpression() {
    return nullptr;
}

ast* parser::aexpression() {
    return nullptr;
}

