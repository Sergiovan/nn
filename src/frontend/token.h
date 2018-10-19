#pragma once

#include "common/grammar.h"
#include "common/convenience.h"

class reader;

struct token {
    token(reader* r);
    
    Grammar::TokenType type{Grammar::TokenType::NONE};
    std::string value{""};
    
    reader* r{nullptr};
    u64 index{0};
    u64 line{1};
    u64 column{1};
    std::string source{""};
    
    u64 as_integer();
    double as_real();
    utfchar as_char();
    std::string as_string();
    Grammar::Keyword as_keyword();
    Grammar::Symbol as_symbol();
};
