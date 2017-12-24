#pragma once

#include <regex>
#include <queue>

#include "reader.h"
#include "token.h"

class tokenizer {
public:
    tokenizer(reader& r);
    
    std::string get_line(int context_back = -1, int context_forwards = -1, int max_chars = -1);
    std::string get_line_context();
    
    token next(); // DOES consume tokens
    token peek(u32 pos = 0); // Does NOT consume tokens
    token peek_until(Grammar::Symbol symbol);
    token peek_until(std::initializer_list<Grammar::Symbol> symbols);
    token peek_until(Grammar::Symbol symbol, Grammar::Keyword keyword);
    i64 search_lookahead(Grammar::Symbol symbol);
private:
    token read(); // Reads a token, and consumes it

    inline bool is_a(char c, const std::regex& r) { //TODO Optimize?
        return c == EOF ? false : std::regex_match(std::string(1, c), r);
    }

    std::deque<token> lookahead{};
    reader& r;

};
