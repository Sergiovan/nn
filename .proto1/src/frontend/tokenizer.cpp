#include "tokenizer.h"

#include "common/util.h"
#include "common/logger.h"

tokenizer::tokenizer(reader& r) : r(r) {}

std::string tokenizer::get_line(int context_back, int context_forward, int max_chars) {
    return r.get_file_line(context_back, context_forward, max_chars);
}

std::string tokenizer::get_line_context() {
    return r.get_file_position();
}

token tokenizer::next() {
    token t;
    if(!lookahead.empty()) {
        t = lookahead.front();
        lookahead.pop_front();
    } else {
        t = read();
    }

    return t;
}

token tokenizer::peek(u32 pos) {
    if(pos < lookahead.size()) {
        return lookahead[pos];
    } else {
        while(pos >= lookahead.size()) {
            lookahead.push_back(read());
        }
        return lookahead.back();
    }
}

token tokenizer::peek_until(Grammar::Symbol symbol) {
    using namespace Grammar;
    token tok = read();
    while(true) {
        lookahead.push_back(tok);
        if(tok.tokenType == TokenType::END_OF_FILE) {
            break;
        }
        if(tok.tokenType == TokenType::SYMBOL) {
            if(tok.to_symbol() == symbol) {
                break;
            }
        }
        tok = read();
    }
    return lookahead.back();
}


token tokenizer::peek_until(std::initializer_list<Grammar::Symbol> symbols) {
    using namespace Grammar;
    token tok = read();
    while(true) {
        lookahead.push_back(tok);
        if(tok.tokenType == TokenType::END_OF_FILE) {
            break;
        }
        if(tok.tokenType == TokenType::SYMBOL) {
            for(auto& symbol : symbols) {
                if (tok.to_symbol() == symbol) {
                    break;
                }
            }
        }
        tok = read();
    }
    return lookahead.back();
}

token tokenizer::peek_until(Grammar::Symbol symbol, Grammar::Keyword keyword) {
    using namespace Grammar;
    token tok = read();
    while(true) {
        lookahead.push_back(tok);
        if(tok.tokenType == TokenType::END_OF_FILE) {
            break;
        }
        if(tok.tokenType == TokenType::SYMBOL) {
            if(tok.to_symbol() == symbol) {
                break;
            }
        }
        if(tok.tokenType == TokenType::KEYWORD) {
            if(tok.to_keyword() == keyword) {
                break;
            }
        }
        tok = read();
    }
    return lookahead.back();
}

i64 tokenizer::search_lookahead(Grammar::Symbol symbol) {
    using namespace Grammar;
    i64 elem = 0;
    for(auto& token : lookahead) {
        if(token.tokenType == TokenType::SYMBOL && token.to_symbol() == symbol) {
            return elem;
        }
        ++elem;
    }
    return -1;
}

token tokenizer::read() {
    using namespace Grammar;

    static const std::regex letter("[a-zA-Z]");
    static const std::regex number("[0-9]");
    static const std::regex whitespace("[ \\n\\r\\t]");
    static const std::regex symbol("[+\\-*\\/%#~&@|^!?:;<>,.'\"[\\]{}()=\\\\`]");

    static const std::regex iden_start("[^ 0-9\\n\\r\\t+\\-*\\/%#~&@|^!?:;<>,.'\"[\\]{}()=\\\\`]");
    static const std::regex iden("[^ \\n\\r\\t+\\-*\\/%#~&@|^!?:;<>,.'\"[\\]{}()=\\\\`]");

    token t;

    char c = r.next();

    /* Remove all the useless crap */
    while (is_a(c, whitespace) || (c == '#' || (c == '/' && r.peek() == '/')) || (c == '/' && r.peek() == '*')) {
        if(is_a(c, whitespace)) {
            /* Remove whitespace */
            do {
                c = r.next();
            } while(is_a(c, whitespace)); // Will end in case of EOF
        } else if(c == '#' || (c == '/' && r.peek() == '/')) {
            /* Single line */
            while(r.next() != '\n' && !r.has_finished()); // Discard all input till newline or end
            c = r.next();
        } else if(c == '/' && r.peek() == '*') {
            /* Multiline */
            int depth = 1; // Fucking nested comments, of course
            do {
                c = r.next();
                if(c == '\\') {
                    r.next(); // Skip next character
                    c = r.next();
                }
                if(c == '*' && r.peek() == '/') {
                    r.next();
                    depth--;
                } else if (c == '/' && r.peek() == '*') {
                    r.next();
                    depth++;
                }
            } while(depth > 0 && !r.has_finished());
            if(r.has_finished() && depth != 0) {
                Logger::warning("Multiline comment not closed properly");
            }
            c = r.next();
        }
    }

    /* What are you still doing here */
    if(r.has_finished()) {
        t.tokenType = TokenType::END_OF_FILE;
        return t;
    }

    /* Numbers? */
    if(is_a(c, number)) {
        t.value += c;
        while(is_a(r.peek(), number)) {
            t.value += r.next();
        }
        if(r.peek() == '.') { // Decimals!
            t.value += r.next();
            while(is_a(r.peek(), number)) {
                t.value += r.next();
            }
        }

        t.tokenType = TokenType::NUMBER;
        return t;
    }

    /* String literals */
    if(c == '"') {
        c = r.next();
        do {
            if(c == '\\' && r.peek() != EOF) {
                c = r.next();
                switch(c) {
                    case 'n':
                        t.value += '\n';
                        break;
                    case 't':
                        t.value += '\t';
                        break;
                    case '0':
                        t.value += '\0';
                        break;
                    default: //TODO Hex literals and that stuff
                        t.value += c;
                        break;
                }
            } else if (c == EOF) {
                Logger::error("String literal extends past end of file");
            } else {
                t.value += c;
            }
            c = r.next();
        } while(c != '"' && !r.has_finished());

        t.tokenType = TokenType::STRING;
        return t;
    }

    /* Char literals */
    if(c == '\'') {
        char utf = r.next();
        unsigned char len = Util::utf8_length(utf) - 1;
        t.value += utf;
        for(unsigned char i = 0; i < std::min(len, (unsigned char) 3); ++i) {
            t.value += r.next();
        }
        r.next(); // Discard ' //TODO Fukin make sure it's a '

        t.tokenType = TokenType::CHARACTER;
        return t;
    }

    /* Symbols */
    if(is_a(c, symbol)) {
        std::string sym(1, c);
        auto s = string_to_symbol.find(sym.c_str());
        if(s != string_to_symbol.end()) {
            t.value = sym;
            t.tokenType = TokenType::SYMBOL;
        }
        while(is_a(r.peek(), symbol) && !r.has_finished()) {
            sym += r.peek();
            s = string_to_symbol.find(sym.c_str());
            if(s != string_to_symbol.end()) {
                t.value = sym;
                t.tokenType = TokenType::SYMBOL;
            } else if(t.tokenType != TokenType::NONE) {
                break;
            }
            r.next();
        }

        return t; // Might be a none-token, but that's fine, lexer will solve that
    }

    /* Compiler idens */
    if(c == '$') { // Compiler identifier
        if(is_a(r.peek(), iden_start)) {
            t.value += c;
            do {
                t.value += r.next();
            } while(is_a(r.peek(), iden) && !r.has_finished());

            t.tokenType = TokenType::COMPILER_IDENTIFIER;
            return t;
        } else {
            t.tokenType = TokenType::SYMBOL;
            t.value = "$";
            return t;
        }
    }

    /* Identifiers and keywords! */
    if(is_a(c, iden_start)) {
        t.value += c;
        while(is_a(r.peek(), iden) && !r.has_finished()) {
            t.value += r.next();
        }

        auto k = string_to_keyword.find(t.value.c_str());
        if(k != string_to_keyword.end()) {
            t.tokenType = TokenType::KEYWORD;
        } else {
            t.tokenType = TokenType::IDENTIFIER;
        }
        return t;
    }

    /* Something has gone terribly wrong */
    Logger::error(c, " has gone past all ifs. That's no bueno");

    t.value += c;

    return t;
}