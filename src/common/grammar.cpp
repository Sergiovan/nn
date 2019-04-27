#include "grammar.h"

std::ostream& operator<<(std::ostream& os, Grammar::TokenType tt) {
    return os << Grammar::tokentype_names.at(tt);
}

std::ostream& operator<<(std::ostream& os, Grammar::Symbol sym) {
    return os << Grammar::symbol_names.at(sym);
}

std::ostream& operator<<(std::ostream& os, Grammar::Keyword kw) {
    return os << Grammar::keyword_names.at(kw);
}
