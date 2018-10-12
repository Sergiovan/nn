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

    auto& petype = etype();
    push_ctx({TypeID::LET}, new symbol_table{&st()});
    CTX_GUARD;
    ast* forast = new ast{ast_node_binary{Symbol::KWFOR, nullptr, nullptr, 0, false}};
    forast->get_binary().left = forcond();

    etype() = petype;
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

    ast* forstart = nullptr;

    if(is(Symbol::SEMICOLON)) {
        type = 0;
    } else if(is_varclass() || is_type() || is(Keyword::LET)) {
        ast* vardeclperiodast = vardeclperiod();
        if(is(Symbol::COLON)) {
            type = 1;
            forstart = vardeclperiodast;
        } else {
            ast* vardeclassast = vardeclass();
            if(is(Symbol::BRACE_LEFT) || is(Keyword::DO)) {
                type = 2;
            } else {
                type = 0;
            }
            forstart = new ast{ast_node_binary{Symbol::ASSIGN, vardeclperiodast, vardeclassast}};
        }
    } else if(is(TokenType::IDENTIFIER)) {
        forstart = assignment();
        if(is(Symbol::BRACE_LEFT) || is(Keyword::DO)) {
            type = 2;
        } else {
            type = 0;
        }
    } else {
        type = 0;
        forstart = fexpression();
    }


    switch(type) {
        case 0: { // We are at the first semicolon
            ast* allforit = new ast{ast_node_block{{}, &st()}};
            auto& stmts = allforit->get_block().stmts;
            stmts.push_back(forstart);

            require(Symbol::SEMICOLON, "for is missing a ;"); // ;
            next();

            if(!is(Symbol::SEMICOLON)) {
                stmts.push_back(expression());
            } else {
                stmts.push_back(nullptr);
            }

            require(Symbol::SEMICOLON, "for is missing a ;");
            next();

            if(!is(Symbol::BRACE_LEFT) && !is(Keyword::DO)) { // { do
                stmts.push_back(mexpression());
            } else {
                stmts.push_back(nullptr);
            }

            forcondast->get_unary().node = allforit;
            forcondast->get_unary().op = Symbol::KWFORCLASSIC;

            break;
        }
        case 1: { // We are at the colon
            ast* forcolon = new ast{ast_node_binary{Symbol::COLON, forstart, nullptr}};
            ast_node_binary& bin = forcolon->get_binary();
            etype() = bin.left->get_type(); // TODO Check this works, check arrayness

            require(Symbol::COLON, "for is missing a :"); // :
            next();

            bin.right = expression();
            etype() = {TypeID::LET};
            forcondast->get_unary().node = forcolon;
            forcondast->get_unary().op = Symbol::KWFOREACH;
            break;
        }
        case 2: { // We are... done?
            if (forstart->type != NodeType::BINARY || forstart->get_binary().op != Symbol::ASSIGN) {
                throw parser_exception(t, "Illegal LUA for format");
            }
            ast_node_binary& luablock = forstart->get_binary();
            if(luablock.left->type != NodeType::PRE_UNARY || luablock.right->type != NodeType::BLOCK) {
                throw parser_exception(t, "Illegal LUA for format");
            }
            ast_node_block& commas = luablock.right->get_block();
            if(commas.stmts.size() != 2 && commas.stmts.size() != 3) {
                throw parser_exception(t, "Illegal LUA for format");
            }
            forcondast->get_unary().node = forstart;
            forcondast->get_unary().op = Symbol::KWFORLUA;
            break;
        }
        default:
            throw parser_error("Illegal for type");
    }

    return forcondast;
}

ast* parser::whilestmt() {
    require(Keyword::WHILE, "Expected while");
    next();

    ast* whileast = new ast{ast_node_binary{Symbol::KWWHILE, nullptr, nullptr}};

    auto& eptype = etype();
    push_ctx({TypeID::LET}, new symbol_table{&st()});
    CTX_GUARD;

    whileast->get_binary().left = new ast{ast_node_block{{}, &st()}};
    auto& stmts = whileast->get_binary().left->get_block().stmts;

    do {
        stmts.push_back(fexpression());
    } while(is(Symbol::SEMICOLON)); // ;

    if(!boolean(stmts.back()->get_type())) {
        throw parser_exception(t, "Last statement in while condition can not be converted to boolean");
    }

    etype() = eptype;

    if(is(Symbol::BRACE_LEFT)) {
        whileast->get_binary().right = scope();
    } else if(is(Keyword::DO)) {
        next();
        whileast->get_binary().right = scopestatement();
    } else {
        throw parser_exception(t, "Expected { or do to start while");
    }
    return whileast;
}

ast* parser::switchstmt() {
    require(Keyword::SWITCH, "Expected switch");
    next();

    auto& eptype = etype();
    push_ctx({TypeID::LET}, new symbol_table{&st()});
    CTX_GUARD;

    ast* switchast = new ast{ast_node_binary{Symbol::KWSWITCH, nullptr, nullptr}};
    switchast->get_binary().left = new ast{ast_node_block{{}, &st()}};

    auto& stmts = switchast->get_binary().left->get_block().stmts;

    do {
        stmts.push_back(fexpression());
    } while(is(Symbol::SEMICOLON));

    aux = stmts.back()->get_type();
    etype() = eptype;

    switchast->get_binary().right = switchscope();

    aux = {};

    return switchast;
}

ast* parser::switchscope() {
    require(Symbol::BRACE_LEFT, "Expected { to start switch scope");
    next();

    ast* switchscope = new ast{ast_node_block{{}, &st()}};
    auto& stmts = switchscope->get_block().stmts;

    while (!is(Symbol::BRACE_RIGHT)) {
        stmts.push_back(casestmt());
    }

    require(Symbol::BRACE_RIGHT, "Expected } to end switch");
    next();

    return switchscope;
}

ast* parser::casestmt() {
    ast* caseast = new ast{ast_node_binary{Symbol::SYMBOL_INVALID, nullptr, nullptr}};
    ast_node_binary& bin = caseast->get_binary();

    if(is(Keyword::ELSE)) {
        bin.op = Symbol::KWELSE;
        next();
    } else if (is(Keyword::CASE)) {
        bin.op = Symbol::KWCASE;
        next();
        bin.left = new ast{ast_node_block{{}, &st()}};
        auto& stmts = bin.left->get_block().stmts;

        do {
            ast* exp = expression();
            if(exp->get_type().type != aux.type) { // TODO Convertibles
                throw parser_exception(t, "Case clause not the same type as switch type");
            }
            stmts.push_back(exp);
        } while(is(Symbol::COMMA)); // TODO Trailing comma

    } else {
        throw parser_exception(t, "Expected else or case to start switch case");
    }

    push_ctx(etype(), new symbol_table{&st()});
    CTX_GUARD;

    bin.right = new ast{ast_node_unary{Symbol::SYMBOL_INVALID, nullptr}};
    ast_node_unary& un = bin.right->get_unary();

    if(is(Keyword::CONTINUE)) {
        un.op = Symbol::KWCONTINUE;
    } else if(is(Symbol::BRACE_LEFT)) { // {
        un.op = Symbol::KWCASE;
        un.node = scope();
    } else if(is(Keyword::DO)) { //
        next();

        un.op = Symbol::KWCASE;
        un.node = scopestatement();
    } else {
        throw parser_exception(t, "Expected continue, do, or an opening brace");
    }

    return caseast;
}

ast* parser::returnstmt() {
    require(Keyword::RETURN);

    ast* returnast = new ast{ast_node_unary{Symbol::KWRETURN, nullptr}};
    ast_node_unary& un = returnast->get_unary();

    if(peek(Keyword::VOID)){
        next();
        next();
    } else {
        un.node = new ast{ast_node_block{{}, &st()}};
        do {
            next(); // return ,
            un.node->get_block().stmts.push_back(expression());
        } while(is(Symbol::COMMA)); // TODO Trailing comma
    }
    require(Symbol::SEMICOLON, "Expected semicolon at the end of return statement");
    next();

    // TODO Check returns

    return returnast;
}

ast* parser::raisestmt() {
    require(Keyword::RAISE);

    ast* raiseast = new ast{ast_node_unary{Symbol::KWRAISE, nullptr}};
    ast_node_unary& un = raiseast->get_unary();
    un.node = new ast{ast_node_block{{}, &st()}};

    do {
        next(); // raise ,
        un.node->get_block().stmts.push_back(expression());
    } while(is(Symbol::COMMA)); // TODO Trailing comma
    require(Symbol::SEMICOLON, "Expected semicolon at the end of raise statement");
    next();

    // TODO Check returns, automagic variable creation

    return raiseast;
}

ast* parser::gotostmt() {
    require(Keyword::GOTO);
    next();
    // TODO
    next();
    require(Symbol::SEMICOLON);
    next();
    return nullptr;
}

ast* parser::labelstmt() {
    require(Keyword::LABEL);
    next();
    // TODO
    next();
    require(Symbol::SEMICOLON);
    next();
    return nullptr;
}

ast* parser::deferstmt() {
    require(Keyword::DEFER);
    next();

    ast* deferast = new ast{ast_node_unary{Symbol::KWDEFER, expression()}};
    require(Symbol::SEMICOLON, "Expected semicolon at the end of defer statement");
    next();

    return deferast;
}

ast* parser::breakstmt() {
    require(Keyword::BREAK);
    next();

    require(Symbol::SEMICOLON, "Expected semicolon to end break statement");
    next();

    return new ast{ast_node_unary{Symbol::KWBREAK}};
}

ast* parser::continuestmt() {
    require(Keyword::CONTINUE);
    next();

    require(Symbol::SEMICOLON, "Expected semicolon to end continue statement");
    next();

    return new ast{ast_node_unary{Symbol::KWCONTINUE}};
}

ast* parser::leavestmt() {
    require(Keyword::LEAVE);
    next();

    require(Symbol::SEMICOLON, "Expected semicolon to end leave statement");
    next();

    return new ast{ast_node_unary{Symbol::KWLEAVE}};
}

ast* parser::importstmt() {
    require(Keyword::IMPORT);
    next();

    ast* importast = new ast{ast_node_binary{Symbol::KWIMPORT, nullptr, nullptr}};
    ast_node_binary& bin = importast->get_binary();

    if(is(TokenType::STRING)) {
        bin.left = string();
    } else if(is(TokenType::IDENTIFIER)) {
        token tok = next(); // Identifier
        bin.left = new ast{ast_node_string{}};
        u64 length = sizeof(tok.value[0]) * tok.value.length();
        bin.left->get_string().chars = new u8[length];
        std::memcpy(bin.left->get_string().chars, &tok.value[0], length);
        bin.left->get_string().length = length;
    } else {
        throw parser_exception(t, "Expected string or identifier to import");
    }

    if(is(Keyword::AS)) {
        next();
        bin.right = iden();
    }

    // TODO Do import

    return importast;
}

ast* parser::usingstmt() {
    require(Keyword::USING);

    std::vector<std::string> path{};

    do {
        next(); // using .
        require(TokenType::IDENTIFIER, "Invalid using path");
        token tok = next(); // identifier
        path.push_back(tok.value);
    } while(is(Symbol::ACCESS)); // .

    // TODO Merge symbol tables or whatever

    return new ast{ast_node_unary{Symbol::KWUSING}};
}

ast* parser::namespacescope() {
    require(Symbol::BRACE_RIGHT, "Expected { to start namespace");
    next();

    bool broken = false;
    ast* ret = new ast{ast_node_block{{}, &st()}};
    ast_node_block& block = ret->get_block();

    while(true) {
        try {
            if(is(Keyword::USING)) {
                block.stmts.push_back(usingstmt());
            } else if(is(Keyword::NAMESPACE)) {
                block.stmts.push_back(namespacestmt());
            } else if(is_type() || is_varclass() || is(Keyword::LET) || is(Keyword::STRUCT) || is(Keyword::UNION) ||
                      is(Keyword::ENUM)) {
                block.stmts.push_back(declstmt());
            } else if(is(Symbol::BRACE_RIGHT)) { // }
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
        throw parser_exception(t, "Namespace statement is not using, namespace or declaration");
    }

    return ret;
}

ast* parser::namespacestmt() {
    require(Keyword::NAMESPACE);

    std::vector<std::string> path{};

    do {
        next(); // using .
        require(TokenType::IDENTIFIER, "Invalid namespace path");
        token tok = next(); // identifier
        path.push_back(tok.value);
    } while(is(Symbol::ACCESS)); // .

    // TODO Search for namespace and create

    require(Symbol::BRACE_LEFT, "Expected { to start namespace");

    return new ast{ast_node_unary{Symbol::KWNAMESPACE, namespacescope()}};
}

ast* parser::varclass() {
    vflags flags = 0;
    while(is_varclass()) {
        if(is(Keyword::CONST)) {
            flags |= TypeFlag::CONST;
        } else if(is(Keyword::VOLATILE)) {
            flags |= TypeFlag::VOLATILE;
        } else if(is(Keyword::SIGNED)) {
            flags |= TypeFlag::SIGNED;
        } else if(is(Keyword::UNSIGNED)) {
            flags &= ~TypeFlag::SIGNED;
        } else {
            throw parser_error("Illegal varclass");
        }
    }

    return new ast{ast_node_byte{flags}};
}

ast* parser::parsertype() {
    constexpr char ptrs[3][2] = {"!", "?", "+"};
    if(!is_type()) {
        throw parser_exception(t, "Expected type");
    }

    uid ntype = 0;
    std::stringstream master{};

    if(is(TokenType::IDENTIFIER)) {
        ntype = st().search(c.value)->get_type();
        master << ntype;
        next();
    } else if(is(Keyword::FUN)) {
        auto ft = functype();
        ntype = ft->get_dword().data;
        delete ft;
        master << ntype;
    } else {
        ntype = (uid) c.to_keyword();
        master << ntype;
        next();
    }



    if(is(Symbol::POINTER)) {
        master << "*";
        next();
    } else if(is(Symbol::UNIQUE_POINTER) || is(Symbol::SHARED_POINTER) || is(Symbol::WEAK_POINTER)) {
        master << ptrs[(uid) c.to_symbol() - (uid) Symbol::UNIQUE_POINTER];
        next();
    }

    while(is_varclass()) {
        auto flags = varclass(); // TODO Do not allow Signed/Unsigned
        master << (u32) flags->get_byte().data;
        delete flags;
        if(is(Symbol::POINTER)) {
            master << "*";
        } else if(is(Symbol::UNIQUE_POINTER) || is(Symbol::SHARED_POINTER) || is(Symbol::WEAK_POINTER)) {
            master << ptrs[(uid) c.to_symbol() - (uid) Symbol::UNIQUE_POINTER];
        } else {
            throw parser_exception(t, "Expected pointer type");
        }
        next();
    }

    bool empty = false;
    ast* arrays = new ast{ast_node_block{{}, &st()}};
    while(is(Symbol::BRACKET_LEFT)) { // [
        master << "[";
        next();
        if(is(Symbol::BRACKET_RIGHT)) { // ]
            next();
            empty = true;
        } else {
            if(empty) {
                throw parser_exception(t, "Cannot use defined array dimensions after undefined array dimensions");
            } else {
                arrays->get_block().stmts.push_back(expression());
                require(Symbol::BRACKET_RIGHT, "Expected ] to end array dimension"); // ]
                next();
            }
        }
    }

    std::string ts = master.str();
    auto _type = mtt.find(ts);

    ast* typeast = new ast{ast_node_dword{0}};

    if(_type == mtt.end()) {
        // TODO Reduce
        // TODO Make types
    } else {
        typeast->get_dword().data = _type->second;
    }

    if(arrays->get_block().stmts.size()) {
        return new ast{ast_node_binary{Symbol::BRACKET_LEFT, typeast, arrays}};
    } else {
        delete arrays;
        return typeast;
    }
}

ast* parser::propertype() {
    ast* ret = new ast{ast_node_binary{Symbol::ACCESS, nullptr, nullptr}}; // TODO Make Symbol for this

    ret->get_binary().left = varclass();
    ret->get_binary().right = parsertype();

    return ret;
}

ast* parser::functype() {
    require(Keyword::FUN, "Expected fun to start function type");
    next();

    std::stringstream master;

    if(is(Symbol::THAN_LEFT)) { // < TODO Can we remove this?
        // TODO let
        do {
            next(); // < :
            master << ":";
            auto tp = propertype();
            auto& bin = tp->get_binary();
            if(bin.right->type != NodeType::DWORD) {
                throw parser_exception(t, "Function type arrays cannot have specified dimensions");
            }
            master << bin.left->get_byte().data << bin.right->get_dword().data;
            delete tp;

        } while(is(Symbol::COLON)); // :

        require(Symbol::PAREN_LEFT, "Function type declaration requires parenthesis for arguments"); // (

        if(!peek(Symbol::PAREN_RIGHT)) {
            do {
                next(); // ( ,
                master << ",";
                auto tp = propertype();
                auto& bin = tp->get_binary();
                if(bin.right->type != NodeType::DWORD) {
                    throw parser_exception(t, "Parameter type arrays cannot have specified dimensions");
                }
                master << bin.left->get_byte().data << bin.right->get_dword().data;

                delete tp;
            } while(is(Symbol::COMMA));
            require(Symbol::PAREN_RIGHT, "Expected ) to end function declaration");
            next();
        } else {
            next(); // (
            next(); // )
            master << ",";
        }

        std::string ts = master.str();
        auto _type = mtt.find(ts);

        ast* typeast = new ast{ast_node_dword{0}};

        if(_type == mtt.end()) {
            mtt[ts] = typeast->get_dword().data = tt().add_type(ts);
        } else {
            typeast->get_dword().data = _type->second;
        }

        require(Symbol::THAN_RIGHT, "Expected > to end function type");
        next();

        return typeast;
    } else {
        return new ast{ast_node_dword{TypeID::FUN}};
    }
}

ast* parser::declstmt() {
    if(is(Keyword::STRUCT)) {
        return structdecl();
    } else if(is(Keyword::UNION)) {
        return uniondecl();
    } else if(is(Keyword::ENUM)) {
        return enumdecl();
    } else {
        // TODO After both funcdecl and vardecl
    }
}

ast* parser::declstructstmt() {
    if(is(Keyword::STRUCT)) {
        return structdecl();
    } else if(is(Keyword::UNION)) {
        return uniondecl();
    } else if(is(Keyword::ENUM)) {
        return enumdecl();
    } else {
        // TODO After both funcdecl and vardeclstruct
    }
}

ast* parser::vardeclperiod() {
    ast* decls = new ast{ast_node_block{{}, &st()}};
    auto& stmts = decls->get_block().stmts;

    do {
        if(is(Symbol::COMMA)) {
            next();
        }

        vflags flags = 0;
        if (is_varclass()) {
            auto f = varclass();
            flags = f->get_byte().data;
            delete f;
        }

        ptype pt = {0, flags};

        if (is(Keyword::LET)) {
            pt.type = TypeID::LET;
            next();
        } else {
            auto t = parsertype();
            if (t->type == NodeType::BINARY) {
                pt.type = t->get_binary().left->get_dword().data;
            } else {
                pt.type = t->get_dword().data;
            }
        }

        require(TokenType::IDENTIFIER, "Variables require a unique name");
        std::string& name = c.value;

        st_entry* var{nullptr};

        if(st().has(name)) {
            throw parser_exception(t, "Redeclaring an identifier");
        } else {
            var = st().add_variable(name, pt.type, nullptr, pt.flags, false);
        }

        ast* decl = new ast{ast_node_unary{Symbol::SYMDECL, new ast{ast_node_symbol{var, name}}}};
        stmts.push_back(decl);

    } while(is(Symbol::COMMA));
    return decls;
}

ast* parser::vardecl() {
    ast* assign = new ast{ast_node_binary{Symbol::ASSIGN, nullptr, nullptr}};
    auto decls = vardeclperiod();
    assign->get_binary().left = decls;
    if(is(Symbol::ASSIGN)) {
        auto vals = vardeclass();
        assign->get_binary().right = vals;

        auto& declstmts = decls->get_block().stmts, valstmts = vals->get_block().stmts;
        size_t ndecls = declstmts.size(), nvals = valstmts.size();

        std::vector<ptype> rvals{};
        for(u64 i = 0; i < nvals; ++i) {
            auto& val = valstmts[i];
            if(val->type == NodeType::BINARY && val->get_binary().op == Symbol::FUN_CALL &&
                tt()[val->get_type().type].type == TypeType::FUNCTION) {
                auto& func = tt()[val->get_type().type].get_func();
                for(auto& ret : func.returns) {
                    rvals.push_back(ret);
                }
            } else {
                rvals.push_back(val->get_type());
            }
        }
        
        i64 begin = ndecls - rvals.size(); // TODO Wrong, in case of functions
        if(begin < 0) {
            throw parser_exception(t, "More values than declarations");
        }
        for(u64 i = begin; i < ndecls; ++i) {
            auto& var = declstmts[i]->get_unary().node->get_symbol().symbol->get_variable();
            auto& val = valstmts[i - begin];

            ptype valtype = val->get_type();
            // TODO Flags and conversions
            if(var.id == TypeID::LET) {
                var.data = val;
                var.id = val->get_type().type;
                var.defined = true;
            } else if (val->type == NodeType::BINARY && val->get_binary().op == Symbol::FUN_CALL &&
                       tt()[val->get_type().type].type == TypeType::FUNCTION) {
                auto& func = tt()[val->get_type().type].get_func();
                for(u64 j = 0; j < func.returns.size(); ++j) {
                    auto& var = declstmts[i]->get_unary().node->get_symbol().symbol->get_variable();
                    auto& ret = func.returns[j];
                    var.data = new ast{ast_node_binary{Symbol::FUN_RET, val, new ast{ast_node_qword{j}},
                                                       {ret.flags, ret.type}}};
                    var.id = ret.type;
                    var.defined = true;
                    ++i;
                }
            } else if (var.id == val->get_type().type) {
                var.data = val;
                var.defined = true;
            } else {
                throw parser_exception(t, "Assigning value of wrong type");
            }
        }
    }

    return assign;
}

ast* parser::vardeclass() {
    require(Symbol::ASSIGN, "Expected =");
    ast* values = new ast{ast_node_block{{}, &st()}};
    
    do {
        values->get_block().stmts.push_back(aexpression());
    } while(is(Symbol::COMMA));
    
    return values;
}

ast* parser::vardeclstruct() {
    return vardecl(); // TODO Bitfields, eventually
}

ast* parser::funcdecl() {
    type typ{TypeType::FUNCTION, type_func{}};
    type_func& tf = typ.get_func();
    std::stringstream ss;
    auto& rets = tf.returns;
    if(is(Keyword::LET) || is(Keyword::VOID)) {
        rets.push_back(ptype{(uid)c.to_keyword(),0});
        ss << ":" << 0 << (uid)c.to_keyword();
        next();
    } else if(is_type()) {
        do {
            auto pt = parsertype()->get_type();
            rets.push_back(pt);
            ss << ":" << pt.flags << pt.type;
        } while(is(Symbol::COLON));
    } else {
        throw parser_exception(t, "Expected let, void or type");
    }
    
    require(TokenType::IDENTIFIER, "Functions require a unique name");
    std::string& name = c.value;
    
    if(st().has(name)) {
        throw parser_exception(t, "Redeclaring an identifier");
    }
    
    tf.name = name;
    
    next(); // IDEN
    require(Symbol::PAREN_LEFT, "Expected ("); // (
    next();
    
    if(is(Symbol::PAREN_RIGHT)) {
        ss << ",";
    }
    
    while(!is(Symbol::PAREN_RIGHT)) {
        ss << ",";
        auto param = parameter();
        tf.args.push_back(param);
        ss << param.type.flags << param.type.type;
        if(!is(Symbol::COMMA)) {
            require(Symbol::PAREN_RIGHT, "Expected )");
        } else {
            next(); // ,
        }
    }
    
    std::string ts = ss.str();
    auto _type = mtt.find(ts);
    
    ast* decl = new ast{ast_node_binary{Symbol::ASSIGN, nullptr, nullptr}};
    
    if(_type == mtt.end()) {
        mtt[ts] = typeast->get_dword().data = tt().add_type(ts);
    } else {
        typeast->get_dword().data = _type->second;
    }
    
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
