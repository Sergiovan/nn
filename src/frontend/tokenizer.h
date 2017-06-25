#pragma once

#include <regex>

#include "reader.h"
#include "token.h"

class tokenizer {
public:
    tokenizer(reader& r);
    token next();
private:
    inline bool is_a(char c, const std::regex& r) { //TODO Optimize?
        return c == EOF ? false : std::regex_match(std::string(1, c), r);
    }
    
    reader& r;
};
