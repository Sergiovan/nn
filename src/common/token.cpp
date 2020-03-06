#include "common/token.h"

#include <iomanip>
#include "common/token_stream.h"

std::ostream& operator<<(std::ostream& os, const token_type& tt) {
    switch (tt) {
        case token_type::NUMBER:
            return os << "NUMBER";
        case token_type::INTEGER:
            return os << "INTEGER";
        case token_type::FLOATING:
            return os << "FLOATING";
        case token_type::SYMBOL:
            return os << "SYMBOL";
        case token_type::KEYWORD:
            return os << "KEYWORD";
        case token_type::CHARACTER:
            return os << "CHARACTER";
        case token_type::STRING:
            return os << "STRING";
        case token_type::IDENTIFIER:
            return os << "IDENTIFIER";
        case token_type::COMPILER:
            return os << "COMPILER";
        case token_type::WHITESPACE:
            return os << "WHITESPACE";
        case token_type::NEWLINE:
            return os << "NEWLINE";
        case token_type::COMMENT:
            return os << "COMMENT";
        case token_type::END_OF_FILE:
            return os << "EOF";
        case token_type::ERROR: [[fallthrough]];
        default:
            return os << "ERROR";
    }
}

std::ostream& operator<<(std::ostream& os, const token& t) {
    os  << t.f.get_name() << "[" << std::setw(3) << std::setfill('0') << (t.line + 1) 
        << ":" << std::setw(3) << (t.column + 1) << "] ["
        << std::setfill(' ') << std::setw(10) << std::left << t.tt << "] | ";
    
    if (t.tt == token_type::NEWLINE) {
        os << "\\n";
    } else if (t.tt == token_type::WHITESPACE) {
        os << "\\t";
    } else {
        auto elen = std::min(t.content.find_first_of('\n'), 47ul);
        os << t.content.substr(0, std::min(t.content.find_first_of('\n'), 47ul));
        if (t.content.length() > elen) {
            os << "...";
        }
    }
    return os;
}
