#include "frontend/token.h"
#include "frontend/reader.h"


token::token(reader* r) : r(r) {
    index = r->get_index();
    line  = r->get_line();
    column = r->get_column();
    source = r->get_source();
}

u64 token::as_integer() {
    // TODO Literals
    return std::stoll(value);
}

double token::as_real() {
    // TODO Literals
    return std::stod(value);
}

utfchar token::as_char() {
    // TODO UTF-8
    utfchar a{0};
    u8* a_mem = reinterpret_cast<u8*>(&a);
    for(u8 i = 0; i < std::min(4ul, value.length()); ++i) {
        a_mem[i] = value[i];
    }
    return a;
}

Grammar::Keyword token::as_keyword() {
    auto kw = Grammar::string_to_keyword.find(value);
    if(kw != Grammar::string_to_keyword.end()) {
        return kw->second;
    } else {
        return Grammar::Keyword::KEYWORD_INVALID;
    }
}

Grammar::Symbol token::as_symbol() {
    auto sym = Grammar::string_to_symbol.find(value);
    if(sym != Grammar::string_to_symbol.end()) {
        return sym->second;
    } else {
        return Grammar::Symbol::SYMBOL_INVALID;
    }
}
