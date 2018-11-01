#pragma once

#include <queue>

#include "reader.h"
#include "token.h"

class lexer {
public:
    lexer(reader* r); // Takes ownership
    ~lexer();
    
    token next();
    token peek(u64 pos = 0);
    void skip(u64 amount = 1);
private:
    token read();
    token make_token(Grammar::TokenType type, const std::string& value);
    
    std::deque<token> lookahead{};
    reader* r{nullptr};
};
