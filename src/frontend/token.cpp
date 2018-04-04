#include "token.h"

u64 token::to_long() {
    // TODO Literals
    return std::atoll(value.c_str());
}

double token::to_double() {
    // TODO ????
    return std::atof(value.c_str());
}

unsigned int token::to_char() {
    // TODO UTF-8
    unsigned int a;
    char* a_mem = reinterpret_cast<char*>(&a);
    for(unsigned int i = 0; i < std::min(static_cast<decltype(value.length())>(4), value.length()); ++i) {
        a_mem[i] = value.at(i);
    }
    return a;
}

Grammar::Keyword token::to_keyword() {
    auto kw = Grammar::string_to_keyword.find(value.c_str());
    if(kw != Grammar::string_to_keyword.end()) {
        return kw->second;
    } else {
        return Grammar::Keyword::KEYWORD_INVALID;
    }
}

Grammar::Symbol token::to_symbol() {
    auto sym = Grammar::string_to_symbol.find(value.c_str());
    if(sym != Grammar::string_to_symbol.end()) {
        return sym->second;
    } else {
        return Grammar::Symbol::SYMBOL_INVALID;
    }
}

