#include "parser.h"

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

bool parser::is(TokenType type) {
    return test(type, 0);
}

bool parser::peek(TokenType type) {
    return test(type, 1);
}

void parser::require(TokenType type) {
    if(n.tokenType != type) {
        throw parser_exception{};
    }
}


