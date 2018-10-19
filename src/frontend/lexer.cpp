#include "frontend/lexer.h"
#include "common/utils.h"

using namespace Grammar;

inline bool is_letter(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

inline bool is_number(char c) {
    return c >= '0' && c <= '9';
}

inline bool is_whitespace(char c) {
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

inline bool is_symbol(char c) {
    switch (c) {
        case '+': [[fallthrough]];
        case '-' : [[fallthrough]];
        case '*' : [[fallthrough]];
        case '/' : [[fallthrough]];
        case '%' : [[fallthrough]];
        case '#' : [[fallthrough]];
        case '~' : [[fallthrough]];
        case '&' : [[fallthrough]];
        case '@' : [[fallthrough]];
        case '|' : [[fallthrough]];
        case '^' : [[fallthrough]];
        case '!' : [[fallthrough]];
        case '?' : [[fallthrough]];
        case ':' : [[fallthrough]];
        case ';' : [[fallthrough]];
        case '<' : [[fallthrough]];
        case '>' : [[fallthrough]];
        case ',' : [[fallthrough]];
        case '.' : [[fallthrough]];
        case '\'': [[fallthrough]];
        case '"' : [[fallthrough]];
        case '[' : [[fallthrough]];
        case ']' : [[fallthrough]];
        case '{' : [[fallthrough]];
        case '}' : [[fallthrough]];
        case '(' : [[fallthrough]];
        case ')' : [[fallthrough]];
        case '=' : [[fallthrough]];
        case '\\': [[fallthrough]];
        case '`' : 
            return true;
        default:
            return false;
    }
}

inline bool is_iden_start(char c) {
    return !(is_number(c) || is_whitespace(c) || is_symbol(c) || c == EOF);
}

inline bool is_iden(char c) {
    return is_iden_start(c) || is_number(c);
}

lexer::lexer(reader* r) : r(r) {
    
}

lexer::~lexer() {
    delete r;
}

token lexer::next() {
    if (!lookahead.empty()) {
        token ret = lookahead.front();
        lookahead.pop_front();
        return ret;
    } else {
        return read();
    }
}

token lexer::peek(u64 pos) {
    if (pos < lookahead.size()) {
        return lookahead[pos];
    } else {
        while (pos >= lookahead.size() && !r->is_done()) {
            lookahead.push_back(read());
        }
        if (pos >= lookahead.size()) {
            return make_token(TokenType::END_OF_FILE, "");
        } else {
            return lookahead[pos];
        }
    }
}

token lexer::read() {
    token t{r};
    char c = r->next();
    
    bool had_whitespace = false;
    
    while (is_whitespace(c) || (c == '#')) {
        had_whitespace = true;
        if (is_whitespace(c)) {
            c = r->next();
            continue;
        } else { // Comment
            if (r->peek() == '>') { // Multiline
                u32 depth = 1;
                c = r->next(); // >
                while (!r->is_done()) {
                    c = r->next();
                    if (c == '#' && r->peek() == '>') {
                        c = r->next();
                        ++depth;
                    } else if (c == '<' && r->peek() == '#') {
                        c = r->next(); // #
                        c = r->next();
                        --depth;
                        if (!depth) {
                            break;
                        }
                    }
                }
                if (depth > 0) {
                    logger::warn() << r->get_file_line() << " - Multiline comment not closed" << logger::nend;
                }
            } else { // Single line
                while (r->next() != '\n' && !r->is_done());
                c = r->next(); // Skip newline
            }
        }
    }
    
    if (had_whitespace) {
        t.index  = r->get_prev_index();
        t.line   = r->get_prev_line();
        t.column = r->get_prev_column();
    }
    
    if (r->is_done()) {
        return make_token(TokenType::END_OF_FILE, "");
    }
    
    // All whitespace and comments removed
    
    // NUMBERS!
    if (is_number(c)) { // TODO Literals
        t.value += c;
        while (is_number(r->peek())) {
            t.value += r->next();
        }
        if (r->peek() == '.') {
            t.value += r->next();
            while (is_number(r->peek())) {
                t.value += r->next();
            }
        }
        t.type = TokenType::NUMBER;
        return t;
    }
    
    // STRINGS!
    if (c == '"') {
        c = r->next();
        do {
            if (c == '\\' && r->peek() != EOF) {
                c = r->next();
                switch (c) {
                    case 'n':
                        t.value += '\n';
                        break;
                    case 't':
                        t.value += '\t';
                        break;
                    case '0':
                        t.value += '\0';
                        break;
                    default:
                        t.value += c;
                        break;
                }
            } else if (c == EOF) {
                logger::warn() << "String literal extends past end of file" << logger::nend;
                break;
            } else {
                t.value += c;
            }
            c = r->next();
        } while (c != '"' && !r->is_done());
        
        t.type = TokenType::STRING;
        return t;
    }
    
    // CHARS!
    if (c == '\'') {
        c = r->next(); // '
        u8 len = utils::utflen(c);
        t.value += c;
        for (u8 i = 0; i < len; ++i) {
            c = r->next();
            if (c == '\\' && r->peek() != EOF) {
                c = r->next();
                switch (c) {
                    case 'n':
                        t.value += '\n';
                        break;
                    case 't':
                        t.value += '\t';
                        break;
                    case '0':
                        t.value += '\0';
                        break;
                    default:
                        t.value += c;
                        break;
                }
            } else if (c == EOF) {
                logger::warn() << "Char literal extends past end of file" << logger::nend;
                break;
            } else {
                t.value += c;
            }
        }
        c = r->next(); // '
        if (c != '\'') {
            logger::error() << "Char literal invalid" << logger::nend;
            return t; // Returns invalid token
        }
        t.type = TokenType::CHARACTER;
        return t;
    }
    
    // SYMBOLS!
    if (is_symbol(c)) {
        std::string sym{c};
        auto s = string_to_symbol.find(sym);
        if (s != string_to_symbol.end()) {
            t.value = sym;
            t.type = TokenType::SYMBOL;
        }
        while (is_symbol(r->peek()) && !r->is_done()) {
            sym += r->peek();
            s = string_to_symbol.find(sym);
            if (s != string_to_symbol.end()) {
                t.value = sym;
                t.type = TokenType::SYMBOL;
            } else if (t.type != TokenType::NONE) {
                break;
            }
            c = r->next();
        }
        return t;
    }
    
    // COMPILER!
    if (c == '$') {
        if (is_iden_start(r->peek())) {
            t.value += c;
            while (is_iden_start(r->peek())) {
                t.value += r->next();
            }
            t.type = TokenType::COMPILER_IDENTIFIER;
            // TODO Lexer compiler identifiers?
        } else {
            t.type = TokenType::SYMBOL;
            t.value = "$";
        }
        return t;
    }
    
    // IDENTIFIERS!
    if (is_iden_start(c)) {
        t.value += c;
        while (is_iden(r->peek())) {
            t.value += r->next();
        }
        
        auto kyw = string_to_keyword.find(t.value);
        if (kyw != string_to_keyword.end()) {
            t.type = TokenType::KEYWORD;
        } else {
            t.type = TokenType::IDENTIFIER;
        }
        
        return t;
    }
    
    logger::error() << c << " could not be fit into any token types" << logger::nend;
    
    t.value += c;
    
    return t;
}

token lexer::make_token(TokenType type, const std::string& value) {
    token t{r};
    t.type = type;
    t.value = value;
    return t;
}
