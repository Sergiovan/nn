#include "tokenizer.h"

#include "common/util.h"
#include "common/logger.h"

tokenizer::tokenizer(reader& r) : r(r) {}

token tokenizer::next() {
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
            t.value = r.next();
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
        auto s = string_to_symbol.get(sym.c_str());
        if(s) {
            t.value = sym;
            t.tokenType = TokenType::SYMBOL;
        }
        while(is_a(r.peek(), symbol) && !r.has_finished()) {
            sym += r.peek();
            s = string_to_symbol.get(sym.c_str());
            if(s) {
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
        
        auto k = string_to_keyword.get(t.value.c_str());
        if(k) {
            t.tokenType = TokenType::KEYWORD;
        } else {
            t.tokenType = TokenType::IDENTIFIER;
        }
        return t;
    }
    
    /* Something has gone terribly wrong */
    Logger::debug(c, " has gone past all ifs. That's no bueno");
    
    t.value += c;
    return t;
}
