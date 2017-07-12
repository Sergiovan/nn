#pragma once

#include <variant>

#include "common/grammar.h"
#include "common/convenience.h"

struct token {
    Grammar::TokenType tokenType = Grammar::TokenType::NONE;
    std::string value = "";
    
    u64 to_long();
    double to_double();
    unsigned int to_char();
    Grammar::Keyword to_keyword();
    Grammar::Symbol to_symbol();
};
