#pragma once

#include <variant>

#include "common/grammar.h"

struct token {
    Grammar::TokenType tokenType = Grammar::TokenType::NONE;
    std::string value = "";
    
    long long to_long();
    long double to_double();
    unsigned int to_char();
    Grammar::Keyword to_keyword();
    Grammar::Symbol to_symbol();
};
