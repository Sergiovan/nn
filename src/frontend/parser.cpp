#include "parser.h"
#include "common/type_table.h"
#include "common/convenience.h"

#include <vector>
#include <common/util.h>

using namespace Grammar;


parser::parser(globals& g) : t(nullptr), g(g) { }

ast* parser::parse(tokenizer& t) {
    parser::t = &t;
    c = t.next();
    n = t.next();
    
    g.init();
}

token parser::next() noexcept {
    token r = c;
    c = n;
    n = t->next();
    return r;
}

bool parser::test(TokenType type, int lookahead) noexcept {
    return (lookahead ? n : c).tokenType == type; // I am a monster
}

bool parser::test(Keyword keyword, int lookahead) noexcept {
    token& t = lookahead ? n : c;
    return t.tokenType == TokenType::KEYWORD && t.to_keyword() == keyword;
}

bool parser::test(Symbol symbol, int lookahead) noexcept {
    token& t = lookahead ? n : c;
    return t.tokenType == TokenType::SYMBOL && t.to_symbol() == symbol;
}

bool parser::is(TokenType type) {
    return test(type, 0);
}

bool parser::is(Keyword keyword) {
    return test(keyword, 0);
}

bool parser::is(Symbol symbol) {
    return test(symbol, 0);
}

bool parser::peek(TokenType type) {
    return test(type, 1);
}

bool parser::peek(Keyword keyword) {
    return test(keyword, 1);
}

bool parser::peek(Symbol symbol) {
    return test(symbol, 1);
}

void parser::require(TokenType type) {
    if(!is(type)) {
        throw parser_exception{};
    }
}

void parser::require(Keyword keyword) {
    if(!is(keyword)) {
        throw parser_exception{};
    }
}

void parser::require(Symbol symbol) {
    if(!is(symbol)) {
        throw parser_exception{};
    }
}

// Actual parser stuff

ast* parser::iden() {
    require(TokenType::IDENTIFIER);
    return new ast{ast_node_symbol{g.get_symbol_table().search(next().value)}};
}

ast* parser::compileriden() {
    if(c.value[0] != '$'){
        throw parser_error();
    }
    //TODO ??????
    next();
    return nullptr;
}

ast* parser::number() {
    require(TokenType::NUMBER);
    return new ast{ast_node_qword{next().to_long(), TypeID::LONG}};
}

ast* parser::string() {
    require(TokenType::STRING);
    auto len = c.value.length();
    u8* str = new u8[len];
    std::memcpy(str, &next().value[0], len);
    return new ast(ast_node_string{str, len});
}

ast* parser::character() {
    require(TokenType::CHARACTER);
    auto len = c.value.length();
    u32 ch = 0;
    std::memcpy(&ch, &next().value[0], len);
    return new ast(ast_node_dword{ch});
}

ast* parser::array() {
    require(Symbol::BRACKET_LEFT); // [
    ast* ret = new ast{ast_node_array{nullptr, 0, 0}};
    std::vector<ast*> elems;
    context_guard cg{context};
    
    context = tt()[context].get_array().of; //TODO Throws
    do {
        next(); // [ , 
        if(is(Symbol::BRACKET_RIGHT)) { // ]
            break;
        } else {
            ast* exp = expression();
            if(context == TypeID::LET) {
                context = exp->get_type();
            } else {
                if(context != exp->get_type()) {
                    throw parser_exception(); // TODO Many many things
                }
            }
            elems.push_back(exp);
        }
    } while (is(Symbol::COMMA));
    require(Symbol::BRACKET_RIGHT);
    next();
    
    auto& arr = ret->get_array();
    arr.length = elems.size();
    arr.elements = new ast*[arr.length];
    std::memcpy(arr.elements, &elems[0], arr.length * sizeof(ast*));
    symbol_table& st = parser::st();
    std::string arr_type_name = Util::stringify(context, "[]");
    st_entry* arr_type = st.search(arr_type_name);
    
    if(arr_type) {
        if(arr_type->type == SymbolTableEntryType::TYPE) {
            arr.type = arr_type->get_type();
        } else {
            throw parser_error(); // TODO This is not possible
        }
    } else {
        uid id = tt().add_type(type{TypeType::ARRAY, type_array{context}});
        st.add_type(arr_type_name, id, true);
        arr.type = id;
    }
    return ret;
}

ast* parser::struct_lit() {
    require(Symbol::BRACE_LEFT); // {
    ast* ret = new ast{ast_node_struct{}};
    ast_node_struct& struc = ret->get_struct();
    context_guard cg{context}; // TODO Throws
    
    struc.type = context;
    
    type_struct& struc_type = tt()[context].get_struct(); // TODO Throws
    
    std::vector<ast*> values{struc_type.fields.size()};
    for(u32 i = 0; i < struc_type.fields.size(); ++i) {
        if(struc_type.fields[i].has_value) {
            values[i] = new ast;
            std::memcpy(values[i], struc_type.fields[i].data.data, sizeof(ast));
        }
    }
    
    u32 elem = 0;
    bool named = false;
    context = TypeID::LET;
    do { // TODO Missing values
        next(); // { ,
        ast* qm = expression();
        if(named || (elem == 0 && qm->type == NodeType::SYMBOL)) {
            if(is(Symbol::ASSIGN)) {
                named = true;
                next();
                
                u32 i = 0;
                
                bool found = false;
                for(; i < struc_type.fields.size(); ++i) {
                    if(struc_type.fields[i].name == qm->get_symbol().name) {
                        found = true;
                        context = struc_type.fields[i].type;
                        break;
                    }
                }
    
                if(!found) {
                    throw parser_exception();
                }
                
                values[i] = expression();
                context = TypeID::LET;
            } else {
                if(named) {
                    throw parser_exception(); // TODO
                }
                values[elem] = qm;
                context = struc_type.fields[elem + 1 == values.size() ? 0 : elem + 1].type;
            }
        } else {
            values[elem] = qm;
            context = struc_type.fields[elem + 1 == values.size() ? 0 : elem + 1].type;
        }
        elem++;
    } while(is(Symbol::COMMA)); // ,
    require(Symbol::BRACE_RIGHT);
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
        throw parser_error(); // TODO
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
                break;
            }
        } catch(parser_exception& ex) {
            // TODO
        }
    }
    if(broken) {
        throw parser_error();
    }
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
    } else if(is_literal()) {
        throw parser_exception(); // TODO Not allowed at the start of a statement, I think
    } else if(is_expression()) {
        ast* exp = expression();
        if(is(Symbol::ASSIGN) || is(Symbol::COMMA)) {
            ret = assignment(exp);
        } else {
            ret = exp;
        }
        require(Symbol::SEMICOLON);
        next();
    } else {
        throw parser_exception(); // TODO Of course todo
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
    } else if(is_literal()) {
        throw parser_exception(); // TODO Not allowed at the start of a statement, I think
    } else if(is_expression()) {
        ast* exp = expression();
        if(is(Symbol::ASSIGN) || is(Symbol::COMMA)) {
            ret = assignment(exp);
        } else {
            ret = exp;
        }
        require(Symbol::SEMICOLON);
        next();
    } else {
        throw parser_exception(); // TODO Of course todo
    }
    return ret;
}

ast* parser::compileropts() {
    require(Symbol::COMPILER);
    next();
    require(Symbol::BRACKET_LEFT);
    do {
        next(); // [ ,
        compileriden();
    } while(is(Symbol::COMMA));
    return nullptr;
}

ast* parser::scope() {
    require(Symbol::BRACE_LEFT);
    ast* ret = new ast{ast_node_block{}};
    ast_node_block& block = ret->get_block();
    
    symbol_guard sg{g};
    symbol_table* bst = new symbol_table{sg.nst};
    g.set_symbol_table(bst);
    block.st = bst;
    
    next(); // }
    do {
        try {
            block.stmts.push_back(statement());
        } catch(parser_exception& ex) {
            // TODO
        }
    } while(!is(Symbol::BRACE_RIGHT));
    next(); // }
    
    return ret;
}

ast* parser::ifstmt() {
    require(Keyword::IF);
    next();
    
    ast* ifast = new ast{ast_node_binary{Symbol::KWIF, nullptr, nullptr, 0, false}};
    ifast->get_binary().left = new ast{ast_node_block{}};
    
    symbol_guard sg{g};
    ast* left_block = ifast->get_binary().left;
    left_block->get_block().st = sg.nst;
    auto& stmts = left_block->get_block().stmts;
    
    do {
        stmts.push_back(fexpression());
    } while(is(Symbol::SEMICOLON));
    
    if(!boolean(stmts[stmts.size() - 1]->get_type())) {
        parser_exception(); // TODO
    }
    
    if(is(Symbol::BRACE_LEFT)) {
        ifast->get_binary().right = ifscope();
    } else if(is(Keyword::DO)) {
        next();
        ifast->get_binary().right = scopestatement();
    } else {
        throw parser_exception(); //TODO Oi!
    }
    return ifast;
}

ast* parser::ifscope() {
    require(Symbol::BRACE_LEFT);
    ast* scopeast = scope();
    if(is(Keyword::ELSE)) {
        next();
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
    require(Keyword::FOR);
    next(); // for
    
    symbol_guard sg{g};
    ast* forast = forcond();
    
    if(is(Symbol::BRACE_LEFT)) {
        forast->get_binary().right = scope();
    } else if(is(Keyword::DO)) {
        next();
        forast->get_binary().right = scopestatement();
    } else {
        throw parser_exception(); //TODO Oi!
    }
    
    return forast;
}

// TODO Other 2 forms of for
ast* parser::forcond() {
    int type = -1;
    if(is(Symbol::SEMICOLON)) {
        type = 0 ; // ; expression ; expression
    } else if(is_fexpression()) {
        type = 0;
    }
    
    switch(type) {
        case 0: {
            ast* classicast = new ast{ast_node_block{}};
            auto& block = classicast->get_block();
            block.st = &st();
            if(is(Symbol::SEMICOLON)) {
                block.stmts.push_back(new ast);
            } else {
                block.stmts.push_back(fexpression());
            }
            next(); // ;
            if(is(Symbol::SEMICOLON)) {
                block.stmts.push_back(new ast);
            } else {
                block.stmts.push_back(expression());
            }
            next(); // ;
            if(is(Symbol::BRACE_LEFT) || is(Keyword::DO)) {
                block.stmts.push_back(new ast);
            } else {
                block.stmts.push_back(mexpression());
            }
            return classicast;
        }
        case -1: [[fallthrough]];
        default:
            throw parser_exception(); //TODO
    }
}

ast* parser::whilestmt() {
    require(Keyword::WHILE);
    next();
    
    ast* whileast = new ast{ast_node_binary{Symbol::KWWHILE, nullptr, nullptr, 0, false}};
    whileast->get_binary().left = new ast{ast_node_block{}};
    
    symbol_guard sg{g};
    ast* left_block = whileast->get_binary().left;
    left_block->get_block().st = sg.nst;
    auto& stmts = left_block->get_block().stmts;
    
    do {
        stmts.push_back(fexpression());
    } while(is(Symbol::SEMICOLON));
    
    if(!boolean(stmts[stmts.size() - 1]->get_type())) {
        parser_exception(); // TODO
    }
    
    if(is(Symbol::BRACE_LEFT)) {
        whileast->get_binary().right = scope();
    } else if(is(Keyword::DO)) {
        next();
        whileast->get_binary().right = scopestatement();
    } else {
        throw parser_exception(); //TODO Oi!
    }
    return whileast;
}

ast* parser::switchstmt() {
    require(Keyword::SWITCH);
    next();
    
    ast* switchast = new ast{ast_node_binary{Symbol::KWSWITCH, nullptr, nullptr, 0, false}};
    switchast->get_binary().left = new ast{ast_node_block{}};
    
    symbol_guard sg{g};
    ast* left_block = switchast->get_binary().left;
    left_block->get_block().st = sg.nst;
    auto& stmts = left_block->get_block().stmts;
    
    do {
        stmts.push_back(fexpression());
    } while(is(Symbol::SEMICOLON));
    
    switchast->get_binary().right = switchscope();
    return switchast;
    
}

ast* parser::switchscope() {
    require(Symbol::BRACE_LEFT);
    next();
    
    ast* switchscope = new ast{ast_node_block{}};
    auto& block = switchscope->get_block();
    block.st = &st();
    
    while(!is(Symbol::BRACE_RIGHT)) {
        block.stmts.push_back(casestmt());
    }
    
    return switchscope;
}

ast* parser::casestmt() {
    ast* caseast = new ast{ast_node_binary{Symbol::SYMBOL_INVALID, nullptr, nullptr, 0, false}};
    auto& binary = caseast->get_binary();
    
    if(is(Keyword::ELSE)) {
        binary.op = Symbol::KWELSE;
    } else if(is(Keyword::CASE)) {
        binary.op = Symbol::KWCASE;
        
    }
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

ast* parser::varclassstmt() {
    return nullptr;
}

ast* parser::typeestmt() {
    return nullptr;
}

ast* parser::functypestmt() {
    return nullptr;
}

ast* parser::declstmt() {
    return nullptr;
}

ast* parser::declstructstmt() {
    return nullptr;
}

ast* parser::vardeclperiod() {
    return nullptr;
}

ast* parser::vardecl() {
    return nullptr;
}

ast* parser::vardeclstruct() {
    return nullptr;
}

ast* parser::funcdecl() {
    return nullptr;
}

ast* parser::parameter() {
    return nullptr;
}

ast* parser::funcval() {
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

ast* parser::assignment(ast* assignee) {
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

ast* parser::e17p() {
    return nullptr;
}

ast* parser::e16() {
    return nullptr;
}

ast* parser::e16p() {
    return nullptr;
}

ast* parser::e15() {
    return nullptr;
}

ast* parser::e15p() {
    return nullptr;
}

ast* parser::e14() {
    return nullptr;
}

ast* parser::e14p() {
    return nullptr;
}

ast* parser::e13() {
    return nullptr;
}

ast* parser::e13p() {
    return nullptr;
}

ast* parser::e12() {
    return nullptr;
}

ast* parser::e12p() {
    return nullptr;
}

ast* parser::e11() {
    return nullptr;
}

ast* parser::e11p() {
    return nullptr;
}

ast* parser::e10() {
    return nullptr;
}

ast* parser::e10p() {
    return nullptr;
}

ast* parser::e9() {
    return nullptr;
}

ast* parser::e9p() {
    return nullptr;
}

ast* parser::e8() {
    return nullptr;
}

ast* parser::e8p() {
    return nullptr;
}

ast* parser::e7() {
    return nullptr;
}

ast* parser::e7p() {
    return nullptr;
}

ast* parser::e6() {
    return nullptr;
}

ast* parser::e6p() {
    return nullptr;
}

ast* parser::e5() {
    return nullptr;
}

ast* parser::e5p() {
    return nullptr;
}

ast* parser::e4() {
    return nullptr;
}

ast* parser::e4p() {
    return nullptr;
}

ast* parser::e3() {
    return nullptr;
}

ast* parser::e2() {
    return nullptr;
}

ast* parser::e2p() {
    return nullptr;
}

ast* parser::e1() {
    return nullptr;
}

ast* parser::e1p() {
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

bool parser::boolean(uid type) {
    auto& t = tt().get_type(type);
    
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
        case TypeType::ENUM: [[fallthrough]];
        case TypeType::ARRAY: return false;
    }
}

parser::symbol_guard::symbol_guard(globals& g) : g(g) {
    ost = &g.get_symbol_table();
    nst = new symbol_table{ost};
}

parser::symbol_guard::~symbol_guard() {
    g.set_symbol_table(ost);
}

parser::context_guard::context_guard(uid &context) : context(context), pcontext(context) { }

parser::context_guard::~context_guard() {
    context = pcontext;
}