#pragma once

#include <string>
#include <iostream>
#include "common/list.h"
#include "grammar.h"

class token_stream;

enum class token_type {
    NUMBER, INTEGER, FLOATING, SYMBOL, 
    KEYWORD, CHARACTER, STRING,
    IDENTIFIER, COMPILER,
    
    WHITESPACE, NEWLINE, COMMENT,
    ERROR, END_OF_FILE
};

struct token : public list_node<token> {
    std::string content;
    
    token_type tt;
    u64 value;
    
    token_stream& f;
    u64 index;
    u64 line;
    u64 column;
    
    token* alternate_next{nullptr};
    token* alternate_prev{nullptr};
    
    static token* leftmost(token* a, token* b);
    static token* rightmost(token* a, token* b);
    static std::string text_between(token* a, token* b);
};

std::ostream& operator<<(std::ostream& os, const token_type& tt);
std::ostream& operator<<(std::ostream& os, const token& t);
