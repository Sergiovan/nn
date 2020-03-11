#include "common/token_stream.h"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <streambuf>

#include "common/logger.h"
#include "common/grammar.h"

token_stream::token_stream(const std::string& filename) : name{filename} { }

token_stream::token_stream(const std::string& name, const std::string& content) 
: name{name}, content{content} { }

token_stream::~token_stream() {
    if (buffer) delete [] buffer;
    
    while (head) {
        token* next = head->next;
        delete head;
        head = next;
    }
}

void token_stream::read() {
    if (done) return;
    
    if (content.empty()) {
        namespace fs = std::filesystem;
        
        if (!fs::exists(name)) {
            std::stringstream ss{};
            ss << "File " << name << " could not be found.";
            push_back(new token {
                {nullptr, nullptr},
                .content = ss.str(),
                .tt = token_type::ERROR,
                .value = 0,
                .f = *this,
                .index = 0,
                .line = 1,
                .column = 1
            });
            push_back(new token {
                {nullptr, nullptr},
                .content = "",
                .tt = token_type::END_OF_FILE,
                .value = 0,
                .f = *this,
                .index = 0,
                .line = 1,
                .column = 1
            });
            return;
        }
        
        std::ifstream file{name, std::ifstream::in | std::ifstream::binary};
        content = std::string{(std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>()};
    }
    
    contentptr = 0;
    
    if (!buffer) {
        buffer = new char[32];
        std::memset(buffer, 0, 32);
        buffsize = 32;
        buffptr = 0;
    }
    
    while(!done) {
        read_one();
    }
    
    push_back(new token {
        {nullptr, nullptr},
        .content = "",
        .tt = token_type::END_OF_FILE,
        .value = 0,
        .f = *this,
        .index = contentptr,
        .line = line + 1,
        .column = 0
    });
}


std::string token_stream::get_name() {
    return name;
}

void token_stream::split(token* tok, u64 at) {
    ASSERT(tok->tt != token_type::NEWLINE && tok->tt != token_type::COMMENT, "Illegal split");
    token* ntok = new token {
        {nullptr, nullptr},
        .content = tok->content.substr(at),
        .tt = tok->tt,
        .value = tok->value,
        .f = *this,
        .index = tok->index + at,
        .line = tok->line,
        .column = tok->column + at
    };
    
    insert_after(ntok, tok);
    tok->content = tok->content.substr(0, at);
}

void token_stream::read_one() {
    if (contentptr >= content.length()) {
        done = true;
        return;
    }
    
    using namespace std::string_literals;
    
    buffptr = 0;
    
    u64 start = contentptr;
    char c = content[contentptr++];
    
    switch (c) {
        case ' ':  [[fallthrough]];
        case '\t': [[fallthrough]];
        case '\r': {
            // Whitespace
            add(c);
            do {
                c = content[contentptr];
                
                switch (c) {
                    case ' ': [[fallthrough]];
                    case '\t': [[fallthrough]];
                    case '\r': 
                        break;
                    default:
                        goto end_whitespace; // Get me out of here
                }
                
                add(c);
                ++contentptr;
            } while(contentptr < content.length());
            
            end_whitespace:
            token* t = create(token_type::WHITESPACE, start);
            push_back(t);
            return;
        }
        case '\n': {
            // Newline
            add(c);
            token* t = create(token_type::NEWLINE, start);
            push_back(t);
            
            ++line;
            columnstart = contentptr;
            
            return;
        }
        case '#': {
            u64 rstart = start - columnstart;
            // Comment
            add(c);
            if (contentptr < content.length() && content[contentptr] == '>') {
                // Multiline
                u64 nesting = 1;
                c = content[contentptr++];
                add(c);
                
                while (contentptr < content.length() && nesting) {
                    c = content[contentptr];
                    
                    if (c == '<' && contentptr + 1 < content.length() && content[contentptr+1] == '#') {
                        add(c);
                        c = content[++contentptr];
                        add(c);
                        --nesting;
                    } else if (c == '#' && contentptr + 1 < content.length() && content[contentptr+1] == '>') {
                        add(c);
                        c = content[++contentptr];
                        add(c);
                        ++nesting;
                    } else if (c == '\n') {
                        add(c);
                        columnstart = contentptr + 1;
                        ++line;
                    } else {
                        add(c);
                    }
                    
                    ++contentptr;
                }
                
            } else {
                // Single line
                while (contentptr < content.length()) {
                    c = content[contentptr];
                
                    if (c == '\n') {
                        break;
                    }
                    
                    add(c);
                    ++contentptr;
                }
            }
            token* t = new token { // Manually to avoid issu
                {nullptr, nullptr},
                .content = std::string{buffer, buffer + buffptr},
                .tt = token_type::COMMENT,
                .value = 0,
                .f = *this,
                .index = start,
                .line = line,
                .column = rstart
            };
            push_back(t);
            return;
        }
        case '\'': {
            // Literal
            add(c);
            if (contentptr >= content.length()) {
                goto end_char; // Incomplete char
            }
            c = content[contentptr++];
            switch (c) {
                case '(': { // Literal tuple
                    add(c);
                    token* t = create(token_type::SYMBOL, start, grammar::LITERAL_TUPLE);
                    push_back(t);
                    return;
                }
                case '[': { // Literal array
                    add(c);
                    token* t = create(token_type::SYMBOL, start, grammar::LITERAL_ARRAY);
                    push_back(t);
                    return;
                }
                case '{': { // Literal brace
                    add(c);
                    token* t = create(token_type::SYMBOL, start, grammar::LITERAL_STRUCT);
                    push_back(t);
                    return;
                }
                case '"': {
                    goto string; // >:(
                }
                case '\'': {
                    add(c);
                    while (contentptr < content.length()) {
                        c = content[contentptr++];
                        switch (c) {
                            case '\\': {
                                add(c);
                                if (contentptr >= content.length()) {
                                    goto end_char; // Incomplete char
                                }
                                c = content[contentptr++];
                                add(c);
                                break;
                            }
                            case '\'': {
                                add(c);
                                goto end_char;
                            }
                            default: {
                                add(c);
                                break;
                            }
                        }
                    }
                    end_char:
                    token* t = create(token_type::CHARACTER, start);
                    push_back(t);
                    return;
                }
                default: {
                    add(c);
                    while (contentptr < content.length()) {
                        c = content[contentptr];
                        
                        switch (c) {
                            case ' ': [[fallthrough]];
                            case '\t':[[fallthrough]];
                            case '\r':[[fallthrough]];
                            case '`': [[fallthrough]];
                            case '~': [[fallthrough]];
                            case '!': [[fallthrough]];
                            case '@': [[fallthrough]];
                            case '$': [[fallthrough]];
                            case '%': [[fallthrough]];
                            case '^': [[fallthrough]];
                            case '&': [[fallthrough]];
                            case '*': [[fallthrough]];
                            case '(': [[fallthrough]];
                            case ')': [[fallthrough]];
                            case '-': [[fallthrough]];
                            case '+': [[fallthrough]];
                            case '=': [[fallthrough]];
                            case '[': [[fallthrough]];
                            case ']': [[fallthrough]];
                            case '{': [[fallthrough]];
                            case '}': [[fallthrough]];
                            case '\\':[[fallthrough]];
                            case '|': [[fallthrough]];
                            case ':': [[fallthrough]];
                            case ';': [[fallthrough]];
                            case ',': [[fallthrough]];
                            case '.': [[fallthrough]];
                            case '<': [[fallthrough]];
                            case '>': [[fallthrough]];
                            case '/': [[fallthrough]];
                            case '#': [[fallthrough]];
                            case '?':
                                goto end_literal;
                                break;
                            case '\n': {
                                logger::debug() << "????";
                                goto end_literal;
                                break;
                            }
                            case '"': {
                                ++contentptr;
                                goto string; // Stop it
                            }
                            default:
                                add(c);
                                break;
                        }
                        
                        ++contentptr;
                    }
                    end_literal:
                    token* t = create(token_type::IDENTIFIER, start, 0);
                    push_back(t);
                    return;
                }
            }
        }
        case '"': {
        string:
            // String
            add(c);
            if (contentptr >= content.length()) {
                goto end_string; // Incomplete string
            }
            while (contentptr < content.length()) {
                c = content[contentptr++];
                switch (c) {
                    case '\\': {
                        add(c);
                        if (contentptr >= content.length()) {
                            goto end_string; // Incomplete string
                        }
                        c = content[contentptr++];
                        add(c);
                        break;
                    }
                    case '\n': {
                        --contentptr;
                        goto end_string;
                    }
                    case '\"': {
                        add(c);
                        goto end_string;
                    }
                    default: {
                        add(c);
                        break;
                    }
                }
            }
            
            end_string:
            token* t = create(token_type::STRING, start); 
            push_back(t);
            return;
        } 
        case '`': [[fallthrough]];
        case '~': [[fallthrough]];
        case '!': [[fallthrough]];
        case '@': [[fallthrough]];
        case '%': [[fallthrough]];
        case '^': [[fallthrough]];
        case '&': [[fallthrough]];
        case '*': [[fallthrough]];
        case '(': [[fallthrough]];
        case ')': [[fallthrough]];
        case '-': [[fallthrough]];
        case '+': [[fallthrough]];
        case '=': [[fallthrough]];
        case '[': [[fallthrough]];
        case ']': [[fallthrough]];
        case '{': [[fallthrough]];
        case '}': [[fallthrough]];
        case '\\': [[fallthrough]];
        case '|': [[fallthrough]];
        case ':': [[fallthrough]];
        case ';': [[fallthrough]];
        case ',': [[fallthrough]];
        case '.': [[fallthrough]];
        case '<': [[fallthrough]];
        case '>': [[fallthrough]];
        case '/': [[fallthrough]];
        case '?': {
            // Symbol
            add(c);
            while (contentptr < content.length()) {
                c = content[contentptr];
                
                switch (c) {
                    case '`': [[fallthrough]];
                    case '~': [[fallthrough]];
                    case '!': [[fallthrough]];
                    case '@': [[fallthrough]];
                    case '%': [[fallthrough]];
                    case '^': [[fallthrough]];
                    case '&': [[fallthrough]];
                    case '*': [[fallthrough]];
                    case '(': [[fallthrough]];
                    case ')': [[fallthrough]];
                    case '-': [[fallthrough]];
                    case '+': [[fallthrough]];
                    case '=': [[fallthrough]];
                    case '[': [[fallthrough]];
                    case ']': [[fallthrough]];
                    case '{': [[fallthrough]];
                    case '}': [[fallthrough]];
                    case '\\': [[fallthrough]];
                    case '|': [[fallthrough]];
                    case ':': [[fallthrough]];
                    case ';': [[fallthrough]];
                    case ',': [[fallthrough]];
                    case '.': [[fallthrough]];
                    case '<': [[fallthrough]];
                    case '>': [[fallthrough]];
                    case '/': [[fallthrough]];
                    case '?': {
                        add(c);
                        ++contentptr;
                        break;
                    }
                    default: {
                        goto end_symbol;
                    }
                }
                
            }
            
            end_symbol:
            token* t = create(token_type::SYMBOL, start); // Gotta be separated later
            push_back(t);
            return;
        }
        case '0': [[fallthrough]];
        case '1': [[fallthrough]];
        case '2': [[fallthrough]];
        case '3': [[fallthrough]];
        case '4': [[fallthrough]];
        case '5': [[fallthrough]];
        case '6': [[fallthrough]];
        case '7': [[fallthrough]];
        case '8': [[fallthrough]];
        case '9': {
            // Number
            add(c);
            if (contentptr >= content.length()) {
                push_back(create(token_type::INTEGER, start));
                return;
            }
            
            u64 base = 10;
            if (c == '0') {
                c = content[contentptr];
                switch (c) {
                    case 'x': [[fallthrough]];
                    case 'X': 
                        base = 16;
                        add(c);
                        contentptr++;
                        break;
                    case 'o': [[fallthrough]];
                    case 'O':
                        base = 8;
                        add(c);
                        contentptr++;
                        break;
                    case 'b': [[fallthrough]];
                    case 'B':
                        base = 2;
                        add(c);
                        contentptr++;
                        break;
                    default:
                        break;
                }
            }
            
            switch (base) {
                case 2: {
                    while (contentptr < content.length()) {
                        c = content[contentptr];
                        switch (c) {
                            case '0': [[fallthrough]];
                            case '1': [[fallthrough]];
                            case '_': [[fallthrough]];
                            case '\'': {
                                add(c);
                                ++contentptr;
                                break;
                            }
                            default:
                                goto end_number;
                        }
                    }
                    break;
                }
                case 8: {
                    while (contentptr < content.length()) {
                        c = content[contentptr];
                        switch (c) {
                            case '0': [[fallthrough]];
                            case '1': [[fallthrough]];
                            case '2': [[fallthrough]];
                            case '3': [[fallthrough]];
                            case '4': [[fallthrough]];
                            case '5': [[fallthrough]];
                            case '6': [[fallthrough]];
                            case '7': [[fallthrough]];
                            case '_': [[fallthrough]];
                            case '\'': {
                                add(c);
                                ++contentptr;
                                break;
                            }
                            default:
                                goto end_number;
                        }
                    }
                    break;
                }
                case 16: {
                    while (contentptr < content.length()) {
                        c = content[contentptr];
                        switch (c) {
                            case '0': [[fallthrough]];
                            case '1': [[fallthrough]];
                            case '2': [[fallthrough]];
                            case '3': [[fallthrough]];
                            case '4': [[fallthrough]];
                            case '5': [[fallthrough]];
                            case '6': [[fallthrough]];
                            case '7': [[fallthrough]];
                            case '8': [[fallthrough]];
                            case '9': [[fallthrough]];
                            case 'a': [[fallthrough]];
                            case 'A': [[fallthrough]];
                            case 'b': [[fallthrough]]; 
                            case 'B': [[fallthrough]];
                            case 'c': [[fallthrough]];
                            case 'C': [[fallthrough]];
                            case 'd': [[fallthrough]];
                            case 'D': [[fallthrough]];
                            case 'e': [[fallthrough]];
                            case 'E': [[fallthrough]];
                            case 'f': [[fallthrough]];
                            case 'F': [[fallthrough]];
                            case '_': [[fallthrough]];
                            case '\'': {
                                add(c);
                                ++contentptr;
                                break;
                            }
                            default:
                                goto end_number;
                        }
                    }
                    break;
                }
                case 10: {
                    bool has_exp = false;
                    
                    while (contentptr < content.length()) {
                        c = content[contentptr];
                        
                        switch (c) {
                            case '0': [[fallthrough]];
                            case '1': [[fallthrough]];
                            case '2': [[fallthrough]];
                            case '3': [[fallthrough]];
                            case '4': [[fallthrough]];
                            case '5': [[fallthrough]];
                            case '6': [[fallthrough]];
                            case '7': [[fallthrough]];
                            case '8': [[fallthrough]];
                            case '9': [[fallthrough]];
                            case '\'': [[fallthrough]];
                            case '_': [[fallthrough]];
                            case '.': {
                                has_exp = false;
                                add(c);
                                ++contentptr;
                                break;
                            }
                            case 'e': [[fallthrough]];
                            case 'E': {
                                has_exp = true;
                                add(c);
                                ++contentptr;
                                break;
                            }
                            case '+': [[fallthrough]];
                            case '-': {
                                if (has_exp) {
                                    add(c);
                                    has_exp = false;
                                    ++contentptr;
                                    break;
                                } else {
                                    goto end_number;
                                }
                            }
                            case 'f': [[fallthrough]];
                            case 'F': {
                                add(c);
                                ++contentptr;
                                goto end_number;
                            }
                            default: 
                                goto end_number;
                        }
                        
                    }
                }
            }
            
            end_number:
            token* t = create(token_type::NUMBER, start); // Gotta be separated later
            push_back(t);
            return;
        }
        default: { 
            add(c);
            while (contentptr < content.length()) {
                c = content[contentptr];
                
                switch (c) {
                    case ' ': [[fallthrough]];
                    case '\t':[[fallthrough]];
                    case '\n':[[fallthrough]];
                    case '\r':[[fallthrough]];
                    case '`': [[fallthrough]];
                    case '~': [[fallthrough]];
                    case '!': [[fallthrough]];
                    case '@': [[fallthrough]];
                    case '%': [[fallthrough]];
                    case '^': [[fallthrough]];
                    case '&': [[fallthrough]];
                    case '*': [[fallthrough]];
                    case '(': [[fallthrough]];
                    case ')': [[fallthrough]];
                    case '-': [[fallthrough]];
                    case '+': [[fallthrough]];
                    case '=': [[fallthrough]];
                    case '[': [[fallthrough]];
                    case ']': [[fallthrough]];
                    case '{': [[fallthrough]];
                    case '}': [[fallthrough]];
                    case '\\': [[fallthrough]];
                    case '|': [[fallthrough]];
                    case ':': [[fallthrough]];
                    case ';': [[fallthrough]];
                    case ',': [[fallthrough]];
                    case '.': [[fallthrough]];
                    case '<': [[fallthrough]];
                    case '>': [[fallthrough]];
                    case '/': [[fallthrough]];
                    case '#': [[fallthrough]];
                    case '"': [[fallthrough]];
                    case '?': {
                        goto end_identifier;
                    }
                    default: {
                        add(c);
                        ++contentptr;
                        break;
                    }
                }
            }
            
            end_identifier:
            token* t = create(buffer[0] == '$' ? token_type::COMPILER : token_type::IDENTIFIER, start); // Possibly keyword
            push_back(t);
            return;
        }
    }
}

void token_stream::add(char c) {
    if (buffptr == buffsize) {
        char* buff = buffer;
        buffer = new char[buffsize << 1];
        std::memcpy(buffer, buff, buffsize);
        std::memset(buffer + buffsize, 0, buffsize);
        buffsize <<= 1;
    }
    buffer[buffptr++] = c;
}

token* token_stream::create(token_type tt, u64 start, u64 value) {
    return new token {
        {nullptr, nullptr},
        .content = std::string{buffer, buffer + buffptr},
        .tt = tt,
        .value = value,
        .f = *this,
        .index = start,
        .line = line,
        .column = start - columnstart
    };
}
