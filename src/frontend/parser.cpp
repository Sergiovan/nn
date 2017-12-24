//
// Created by sergi on 24-Dec-17.
//

#include <common/ast.h>
#include <common/logger.h>
#include <common/util.h>
#include "parser.h"

#define CTX_GUARD context_guard ctxg{*this};

using namespace Grammar;

parser_exception::parser_exception() : message("Something went so wrong an error was not even "
                                                       "properly implemented for it") {

}

parser_exception::parser_exception(tokenizer* t, std::string message, int cb, int cf, int max_chars) {
    std::stringstream ss;
    ss << "Error at " << t->get_line_context() << " -> " << message;
    ss << "\n" << t->get_line(cb, cf, max_chars);
    message = ss.str();
}

const char* parser_exception::what() const noexcept {
    return message.c_str();
}

parser_error::parser_error() : message("This should never ever happen") {

}

parser_error::parser_error(std::string str) : message(str) {

}

const char* parser_error::what() const noexcept {
    return message.c_str();
}

context_guard::context_guard(parser& p) : p(p) {}
context_guard::~context_guard() {
    p.pop_ctx();
}

parser::parser(globals& g) : g(g), cx() {}

ast* parser::parse(tokenizer& t) {
    parser::t = &t;

    g.init();
    cx.push({g.get_symbol_table(), 0});
    next();

    //TODO
}

token parser::next() noexcept {
    token tmp = c;
    c = t->next();
    return tmp;
}

bool parser::is(TokenType type) noexcept {
    return c.tokenType == type;
}

bool parser::is(Keyword keyword) noexcept {
    return c.tokenType == TokenType::KEYWORD && c.to_keyword() == keyword;
}

bool parser::is(Grammar::Symbol symbol) noexcept {
    return c.tokenType == TokenType::SYMBOL && c.to_symbol() == symbol;
}

bool parser::peek(TokenType type, u32 lookahead) noexcept {
    return t->peek(lookahead).tokenType == type;
}

bool parser::peek(Keyword keyword, u32 lookahead) noexcept {
    token tok = t->peek(lookahead);
    return tok.tokenType == TokenType::KEYWORD && tok.to_keyword() == keyword;
}

bool parser::peek(Symbol symbol, u32 lookahead) noexcept {
    token tok = t->peek(lookahead);
    return tok.tokenType == TokenType::SYMBOL && tok.to_symbol() == symbol;
}

void parser::require(TokenType type, std::string message) {
    if(!is(type)) {
        throw parser_exception(t, message);
    }
}

void parser::require(Keyword keyword, std::string message) {
    if(!is(keyword)) {
        throw parser_exception(t, message);
    }
}

void parser::require(Symbol symbol, std::string message) {
    if(!is(symbol)) {
        throw parser_exception(t, message);
    }
}

context& parser::pop_ctx() {
    auto& top = cx.top();
    cx.pop();
    return top;
}

void parser::push_ctx(ptype type, symbol_table *st) {
    if(!st) {
        st = cx.top().st;
    }
    cx.push({st, type});
}

ast* parser::iden() {
    require(TokenType::IDENTIFIER, "Expected identifier");
    return new ast{ast_node_symbol{st().search(next().value)}};
}

ast* parser::compileriden() {
    return nullptr; // TODO everything
}

ast* parser::number() {
    require(TokenType::NUMBER, "Expected number");
    return new ast{ast_node_qword{next().to_long(), TypeID::LONG}}; //TODO fit to number
}

ast* parser::string() {
    require(TokenType::STRING, "Expected string");
    auto len = c.value.length();
    u8* str = new u8[len];
    std::memcpy(str, &next().value[0], len);
    return new ast{ast_node_string{str, len}}; // TODO real unicode length
}

ast* parser::character() {
    require(TokenType::CHARACTER, "Expected character");
    auto len = c.value.length();
    u32 ch = 0;
    std::memcpy(&ch, &next().value[0], len);
    return new ast(ast_node_dword{ch}); // TODO Check unicode in tokenizer?
}

ast* parser::array() {
    require(Symbol::BRACKET_LEFT, "Expected [ to start array"); // [
    ast* ret = new ast{ast_node_array{nullptr, 0, 0}};

    std::vector<ast*> elems;
    push_ctx(tt()[etype().type].get_ptr().at); // TODO can throw
    CTX_GUARD;
    do {
        next(); // [ ,
        if(is(Symbol::BRACE_RIGHT)) { // ]
            break;
        } else {
            ast* exp = expression();
            if(etype().type == TypeID::LET) {
                etype() = exp->get_type();
            } else if (etype().type != exp->get_type().type) {  // TODO Type comparison
                throw parser_exception(t, "Array element was not of expected type", c.value.length());
            }
            elems.push_back(exp);
        }
    } while(is(Symbol::COMMA)); // ,
    require(Symbol::BRACE_RIGHT, "Expected ] to end array");
    next();

    auto& arr = ret->get_array();
    arr.length = elems.size();
    arr.elements = new ast*[arr.length];
    std::memcpy(arr.elements, &elems[0], arr.length * sizeof(ast*));

    return ret;
}

ast* parser::struct_lit() {
    require(Symbol::BRACE_LEFT, "Expected { to start struct"); // {
    ast* ret = new ast{ast_node_struct{}};
    ast_node_struct& struc = ret->get_struct();
    struc.type = etype();
    push_ctx(); // TODO This struct in symbol table temporarily
    CTX_GUARD;

    type_struct& struc_type = tt()[struc.type.type].get_struct();

    std::vector<ast*> values{struc_type.fields.size()};
    for(u32 i = 0; i < values.size(); ++i) { // TODO Convenient way to do this
        if(struc_type.fields[i].data) {
            values[i] = new ast;
            std::memcpy(values[i], struc_type.fields[i].data, sizeof(ast*));
        }
    }

    u32 elem = 0;
    bool named = false;
    etype().type = TypeID::VOID;
    do {
        next(); // { ,
        if(named || peek(Symbol::ASSIGN)) { // =
            named = true;
            std::string name = c.value; // Must be the name of a field
            next(); // Field name
            require(Symbol::ASSIGN, "Expected = for named assignment"); // =
            next();

            auto fpos = struc_type.field_names.find(name);
            if(fpos != struc_type.field_names.end()) { // TODO Can throw
                u64 pos = fpos->second;
                etype() = struc_type.fields[pos].type;
                values[pos] = expression();
                etype() = {TypeID::LET};
            } else {
                throw parser_exception(t, "Name does not name a field"); // TODO
            }
        } else {
            etype() = struc_type.fields[elem].type;
            values[elem] = expression();
            etype() = {TypeID::VOID};
        }
        ++elem;
    } while(is(Symbol::COMMA));

    require(Symbol::BRACE_RIGHT); // }
    next();

    struc.elements = new ast*[values.size()];
    std::memcpy(struc.elements, &values[0], values.size() * sizeof(ast*));

    return ret;
}

ast* parser::literal() {
    if(is(TokenType::CHARACTER)) {
        return character();
    } else if(is(TokenType::NUMBER)) {
        return number();
    } else if(is(TokenType::STRING)) {
        return string();
    } else if(is(Symbol::BRACE_LEFT)) {
        return struct_lit();
    } else if(is(Symbol::BRACKET_LEFT)) {
        return array();
    } else {
        throw parser_error("literal() called on non-literal");
    }
}

ast* parser::program() {
    bool broken = false;
    ast* ret = new ast{ast_node_block{}};
    ast_node_block& block = ret->get_block();
    block.st = &st();

    while(true) {
        try {
            if(is(Keyword::USING)) {
                block.stmts.push_back(usingstmt());
            } else if(is(Keyword::NAMESPACE)) {
                block.stmts.push_back(namespacestmt());
            } else if(is_type() || is_varclass() || is(Keyword::LET) || is(Keyword::STRUCT) || is(Keyword::UNION) ||
                                   is(Keyword::ENUM)) {
                block.stmts.push_back(declstmt());
            } else if(is(Keyword::IMPORT)) {
                block.stmts.push_back(importstmt());
            } else if(is(TokenType::END_OF_FILE)) {
                break;
            } else {
                broken = true;
                break; // TODO Don't break, panic instead
            }
        } catch(parser_exception& ex) {
            Logger::error(ex.what()); // TODO
        }
    }
    if(broken) {
        throw parser_exception(t, "Top level statement is not using, namespace or declaration");
    }

    return ret;
}

ast* parser::statement() {
    if (is(TokenType::COMPILER_IDENTIFIER)) {
        compileriden();
    }

    ast* ret;
    if(is(Keyword::IF)) {
        ret = ifstmt();
    } else if(is(Keyword::FOR)) {
        ret = forstmt();
    } else if(is(Keyword::WHILE)) {
        ret = whilestmt();
    } else if(is(Keyword::SWITCH)) {
        ret = switchstmt();
    } else if(is(Keyword::RETURN)) {
        ret = returnstmt();
    } else if(is(Keyword::RAISE)) {
        ret = raisestmt();
    } else if(is(Keyword::GOTO)) {
        ret = gotostmt();
    } else if(is(Keyword::LABEL)) {
        ret = labelstmt();
    } else if(is(Keyword::DEFER)) {
        ret = deferstmt();
    } else if(is(Keyword::BREAK)) {
        ret = breakstmt();
    } else if(is(Keyword::CONTINUE)) {
        ret = continuestmt();
    } else if(is(Keyword::LEAVE)) {
        ret = leavestmt();
    } else if(is(Keyword::USING)) {
        ret = usingstmt();
    } else if(is(Keyword::NAMESPACE)) {
        ret = namespacestmt();
    } else if(is_type() || is_varclass() || is(Keyword::LET) || is(Keyword::STRUCT) || is(Keyword::UNION) ||
              is(Keyword::ENUM)) {
        ret = declstmt();
    } else if(is(Symbol::BRACE_LEFT)) {
        ret = scope();
    } else if(is_expression()) { // Can be expression OR assignment
        t->peek_until(Symbol::SEMICOLON);
        if(t->search_lookahead(Symbol::ASSIGN) != -1) {
            ret = assstmt();
        } else {
            ret = expressionstmt();
        }
    } else {
        throw parser_exception(t, Util::stringify("Unexpected token:", c.value), c.value.length());
    }
    return ret;
}

ast* parser::scopestatement() {
    ast* ret;
    if(is(Keyword::IF)) {
        ret = ifstmt();
    } else if(is(Keyword::FOR)) {
        ret = forstmt();
    } else if(is(Keyword::WHILE)) {
        ret = whilestmt();
    } else if(is(Keyword::SWITCH)) {
        ret = switchstmt();
    } else if(is(Keyword::RETURN)) {
        ret = returnstmt();
    } else if(is(Keyword::RAISE)) {
        ret = raisestmt();
    } else if(is(Keyword::GOTO)) {
        ret = gotostmt();
    } else if(is(Keyword::BREAK)) {
        ret = breakstmt();
    } else if(is(Keyword::CONTINUE)) {
        ret = continuestmt();
    } else if(is(Keyword::LEAVE)) {
        ret = leavestmt();
    } else if(is(Symbol::BRACE_LEFT)) {
        ret = scope();
    } else if(is_expression()) {
        t->peek_until(Symbol::SEMICOLON);
        if(t->search_lookahead(Symbol::ASSIGN) != -1) {
            ret = assstmt();
        } else {
            ret = expressionstmt();
        }
    } else {
        throw parser_exception(t, Util::stringify("Unexpected token:", c.value), c.value.length());
    }
    return ret;
}

ast* parser::compileropts() {
    require(Symbol::COMPILER, "Whops"); // TODO
    next();
    require(Symbol::BRACKET_LEFT);
    do {
        next(); // [ ,
        compileriden();
    } while(is(Symbol::COMMA));
    return nullptr;
}

ast* parser::scope() {
    require(Symbol::BRACE_LEFT, "Expected { to start scope"); // {
    ast* ret = new ast{ast_node_block{}};
    auto& block = ret->get_block();

    push_ctx(etype(), new symbol_table{&st()});
    CTX_GUARD;

    next(); // {
    do {
        try {
            block.stmts.push_back(statement());
        } catch(parser_exception& ex) {
            Logger::error(ex.what()); // TODO
        }
    } while(!is(Symbol::BRACE_RIGHT)); // }

    require(Symbol::BRACE_RIGHT); // }
    next();

    return ret;
}

ast* parser::ifstmt() {
    require(Keyword::IF, "Expected if"); // if
    next();

    ast* ifast = new ast{ast_node_binary{Symbol::KWIF, nullptr, nullptr, 0, false}};
    ifast->get_binary().left = new ast{ast_node_block{}};

    auto& expected = etype();

    push_ctx({TypeID::LET}, new symbol_table{&st()});
    CTX_GUARD;
    ast* left_block = ifast->get_binary().left;
    left_block->get_block().st = &st();

    auto& stmts = left_block->get_block().stmts;

    do {
        stmts.push_back(fexpression());
    } while(is(Symbol::SEMICOLON)); // ;

    if(!boolean(stmts[stmts.size() - 1]->get_type())) {
        throw parser_exception(t, "Last statement of if is not a boolean");
    }

    etype() = expected;

    if(is(Symbol::BRACE_LEFT)) { // {
        ifast->get_binary().right = ifscope();
    } else if(is(Keyword::DO)) {
        next(); // do
        ifast->get_binary().right = scopestatement();
    } else {
        throw parser_exception(t, "Expected { or do to start if");
    }

    return ifast;
}

ast* parser::ifscope() {
    require(Symbol::BRACE_LEFT, "Expected { to start if scope"); // {
    ast* scopeast = scope();

    if(is(Keyword::ELSE)) {
        next(); // else
        ast* elseast = new ast{ast_node_binary{Symbol::KWELSE, nullptr, nullptr, 0, false}};
        ast_node_binary& binary = elseast->get_binary();

        binary.left = scopeast;
        binary.right = scopestatement();
        return elseast;
    } else {
        return scopeast;
    }
}

ast* parser::forstmt() {
    require(Keyword::FOR, "Expected for");
    next(); // for

    push_ctx(etype(), new symbol_table{&st()});
    CTX_GUARD;
    ast* forast = new ast{ast_node_binary{Symbol::KWFOR, nullptr, nullptr, 0, false}};
    forast->get_binary().left = forcond();

    if(is(Symbol::BRACE_LEFT)) {
        forast->get_binary().right = scope();
    } else if(is(Keyword::DO)) {
        next(); // do
        forast->get_binary().right = scopestatement();
    } else {
        throw parser_exception(t, "Expected { or do after for condition");
    }

    return forast;
}

ast* parser::forcond() {
    int type = -1;

    ast* forcondast = new ast{ast_node_unary{Symbol::SYMBOL_INVALID, nullptr, 0, false}};
    forcondast->get_unary().node = new ast{ast_node_block{{}, &st()}};
    auto& stmts = forcondast->get_unary().node->get_block().stmts;
    ast* vardeclperiodast = nullptr;
    ast* vardeclassast = nullptr;
    ast* assignmentast = nullptr;
    ast* fexpressionast = nullptr;

    if(is(Symbol::SEMICOLON)) {
        type = 0;
    } else if(is_type() || is(Keyword::LET)) {
        vardeclperiodast = vardeclperiod();
        if(is(Symbol::COLON)) {
            type = 1;
        } else {
            vardeclassast = vardeclass();
            if(is(Symbol::BRACE_LEFT) || is(Keyword::DO)) {
                type = 2;
            } else {
                type = 0;
            }
        }
    } else if(is(TokenType::IDENTIFIER)) {
        assignmentast = assignment();
        if(is(Symbol::BRACE_LEFT) || is(Keyword::DO)) {
            type = 2;
        } else {
            type = 0;
        }
    } else {
        type = 0;
        fexpressionast = fexpression();
    }

    // UNTANGLE

    switch(type) {
        case 0: // We are at the first semicolon
        case 1: // We are at the colon
        case 2: // We are... done?
        default:
            throw parser_error("Illegal for type");
    }

    return forcondast;
}

bool parser::is_type() {
    return  is(Keyword::BYTE)   ||
            is(Keyword::CHAR)   ||
            is(Keyword::SHORT)  ||
            is(Keyword::INT)    ||
            is(Keyword::LONG)   ||
            is(Keyword::SIG)    ||
            is(Keyword::FLOAT)  ||
            is(Keyword::DOUBLE) ||
            is(Keyword::BOOL)   ||
            is(Keyword::STRING) ||
            is(Keyword::FUN)    ||
            ( is(TokenType::IDENTIFIER) && st().search(c.value)->type == SymbolTableEntryType::TYPE);
}

bool parser::is_varclass() {
    return  is(Keyword::CONST) ||
            is(Keyword::VOLATILE);
}

bool parser::is_literal() {
    return  is(TokenType::CHARACTER) ||
            is(TokenType::NUMBER) ||
            is(TokenType::STRING) ||
            is(Symbol::BRACKET_LEFT) ||
            is(Symbol::BRACE_LEFT);
}

bool parser::is_expression() {
    return  is(TokenType::IDENTIFIER) ||
            is(Symbol::PAREN_LEFT) ||
            is(Symbol::INCREMENT) ||
            is(Symbol::DECREMENT) ||
            is(Symbol::ADD) ||
            is(Symbol::SUBTRACT) ||
            is(Symbol::NOT) ||
            is(Symbol::LNOT) ||
            is(Symbol::THAN_LEFT) ||
            is(Symbol::DEREFERENCE) ||
            is(Symbol::ADDRESS) ||
            is_literal();
}

bool parser::is_fexpression() {
    return is_expression() || is_varclass() || is_type() || is(Keyword::LET);
}

bool parser::boolean(ptype type) {
    auto& t = tt().get_type(type.type);

    switch(t.type) {
        case TypeType::PRIMITIVE:
            switch(t.get_primitive().type){
                case PrimitiveType::VOID: [[fallthrough]];
                case PrimitiveType::CHAR: [[fallthrough]];
                case PrimitiveType::STRING: [[fallthrough]];
                case PrimitiveType::FUN: [[fallthrough]];
                case PrimitiveType::LET: return false;
                case PrimitiveType::BYTE: [[fallthrough]];
                case PrimitiveType::SHORT: [[fallthrough]];
                case PrimitiveType::INT: [[fallthrough]];
                case PrimitiveType::LONG: [[fallthrough]];
                case PrimitiveType::FLOAT: [[fallthrough]];
                case PrimitiveType::DOUBLE: [[fallthrough]];
                case PrimitiveType::__LDOUBLE: [[fallthrough]];
                case PrimitiveType::BOOL: [[fallthrough]];
                case PrimitiveType::SIG: return true;
            }
            throw parser_error(); // TODO Stuff
        case TypeType::POINTER: return true;
        case TypeType::STRUCT: [[fallthrough]];
        case TypeType::FUNCTION: [[fallthrough]];
        case TypeType::UNION: [[fallthrough]];
        case TypeType::ENUM: return false;
    }
    return false;
}
