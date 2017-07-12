#include "parser.h"
#include "common/type_table.h"
#include "common/convenience.h"

#include <vector>

using namespace Grammar;


parser::parser(globals& g) : t(nullptr), g(g) { }

ast* parser::parse(tokenizer& t) {
    parser::t = &t;
    c = t.next();
    n = t.next();
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

ast* parser::iden() {
    require(TokenType::IDENTIFIER);
    return new ast{ast_node_symbol{g.get_symbol_table().search(next().value)}};
}

ast* parser::compileriden() {
    if(c.value[0] != '$'){
        throw parser_exception();
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
    return new ast(ast_node_dword{ch, TypeID::CHAR});
}

ast* parser::array() {
    require(Symbol::BRACKET_LEFT); // [
    ast* ret = new ast{ast_node_array{nullptr, 0}};
    std::vector<ast*> elems;
    do {
        next(); // [ , 
        if(is(Symbol::BRACKET_RIGHT)) { // ]
            break;
        } else {
            elems.push_back(expression());
        }
    } while (is(Symbol::COMMA));
    require(Symbol::BRACKET_RIGHT);
    next();
    auto& arr = ret->get_array();
    arr.length = elems.size();
    arr.elements = new ast*[arr.length];
    std::memcpy(arr.elements, &elems[0], arr.length * sizeof(ast*));
    return ret;
}

ast* parser::struct_lit() {
    return nullptr; //TODO Expression
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

ast* parser::compileropts() {
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

ast* parser::varstmt() {
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

ast* parser::assignment() {
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
