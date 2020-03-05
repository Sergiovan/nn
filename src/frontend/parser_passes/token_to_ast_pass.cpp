#include "frontend/parser_passes/token_to_ast_pass.h"

#include <cmath>
#include <cstring>
#include "common/util.h"
#include "common/logger.h"

using pass = token_to_ast_pass;

pass::token_to_ast_pass(nnmodule& mod) : mod{mod}, tt{mod.tt} {
    
}

void pass::pass() {
    mod.ts.read();
    
    if (mod.ts.head->tt == token_type::ERROR) {
        mod.errors.push_back({make_error_ast(mod.ts.head), mod.ts.head->content});
        return;
    }
    
    n = mod.ts.head;
    clear();
    c = n;
    
    // mod.root = program_unit();
    
    u64 i{0};
    
    while (c) {
        token* tok = next();
        if (tok->tt == token_type::COMMENT || tok->tt == token_type::WHITESPACE || 
            tok->tt == token_type::NEWLINE) {
            // Skip
        } else {
            logger::info() << *tok;
            ++i;
        }
    }
    
    logger::info() << "Counted " << i << " tokens";
    
    return;
}

token* pass::next() {
    return next(grammar::SPECIAL_INVALID);
}

token* pass::next(grammar::symbol expected) {
    using grammar::string_to_symbol;
    
    token* buff = c;
    peek(expected);
    c = n;
    return buff;
}

token* pass::peek(grammar::symbol expected) {
    using grammar::string_to_symbol;
    
    if (n != c) {
        return n;
    }
    
    auto& syms = string_to_symbol;
        
    while (n) {
        token* buff = n;
        n = n->next;
        
        switch (buff->tt) {
            case token_type::NUMBER: [[fallthrough]];
            case token_type::INTEGER: [[fallthrough]];
            case token_type::FLOATING: [[fallthrough]];
            case token_type::CHARACTER: [[fallthrough]];
            case token_type::STRING: [[fallthrough]];
            case token_type::COMPILER: {
                
                clear();
                return buff;
            }
            case token_type::IDENTIFIER: [[fallthrough]];
            case token_type::KEYWORD: {
                if (auto it = string_to_symbol.find(buff->content); it != string_to_symbol.end()) {
                    buff->tt = token_type::KEYWORD;
                    buff->value = it->second;
                }
                
                clear();
                return buff;
            }
            case token_type::SYMBOL: {
                char sym[4] {'\0', '\0', '\0', '\0'};
                u8 i = 0;
                grammar::symbol found{grammar::SPECIAL_INVALID};
                for (; i < std::min(4ul, buff->content.size()); ++i) {
                    sym[i] = buff->content[i];
                    if (auto it = syms.find(sym); it != syms.end()) { // We found a symbol
                        found = (grammar::symbol) it->second;
                        if (found == expected) { // It's the one we expected
                            ++i;
                            goto got_it;
                        }
                    } else if (found != grammar::SPECIAL_INVALID) { // We found no symbol, and we already have one
                        sym[i] = '\0';
                        goto got_it;
                    }
                }
                
                if (found != grammar::SPECIAL_INVALID) {
                    goto got_it;
                }
                
                // Nothing was found, split and error
                if (buff->content.size() > i) {
                    mod.ts.split(buff, i);
                    n = buff->next;
                }
                buff->tt = token_type::ERROR;
                
                clear();
                return buff;
                
                got_it: // We found our symbol, split and return
                u8 len = i;
                if (len < buff->content.length()) {
                    mod.ts.split(buff, len);
                }
                n = buff->next;
                buff->value = found;
                
                clear();
                return buff;
            }
            case token_type::WHITESPACE: [[fallthrough]];
            case token_type::NEWLINE: [[fallthrough]];
            case token_type::COMMENT: 
                break;
            case token_type::ERROR: {
                mod.errors.push_back({make_error_ast(buff), buff->content});
                break;
            }
        }
    }
    
    return n;
}

void pass::clear() {
    while (n) {        
        switch (n->tt) {
            case token_type::NUMBER: [[fallthrough]];
            case token_type::INTEGER: [[fallthrough]];
            case token_type::FLOATING: [[fallthrough]];
            case token_type::CHARACTER: [[fallthrough]];
            case token_type::STRING: [[fallthrough]];
            case token_type::COMPILER: [[fallthrough]];
            case token_type::IDENTIFIER: [[fallthrough]];
            case token_type::KEYWORD: [[fallthrough]];
            case token_type::SYMBOL:
                return;
            case token_type::WHITESPACE: [[fallthrough]];
            case token_type::NEWLINE: [[fallthrough]];
            case token_type::COMMENT: 
                break;
            case token_type::ERROR: {
                mod.errors.push_back({make_error_ast(n), n->content});
                break;
            }
        }
        n = n->next;
    }
}

bool pass::is(token_type tt) {
    return c->tt == tt;
}

bool pass::is_keyword(grammar::symbol kw) {
    return c->tt == token_type::KEYWORD && c->value == kw;
}

bool pass::is_symbol(grammar::symbol sym) {
    return c->tt == token_type::SYMBOL && c->value == sym;
}

bool pass::require(token_type tt) {
    if (c->tt == tt) {
        return true;
    } else {
        mod.errors.push_back({make_error_ast(c), ss::get() << "Expecting " << tt << ss::end()});
        return false;
    }
}

bool pass::require_keyword(grammar::symbol sym) {
    if (c->tt == token_type::KEYWORD && c->value == sym) {
        return true;
    } else {
        mod.errors.push_back({make_error_ast(c), ss::get() << "Expecting " << sym << ss::end()});
        return false;
    }
}

bool pass::require_symbol(grammar::symbol sym) {
    if (c->tt == token_type::SYMBOL && c->value == sym) {
        return true;
    } else {
        mod.errors.push_back({make_error_ast(c), ss::get() << "Expecting " << sym << ss::end()});
        return false;
    }
}

ast* pass::number() {
    token* tok = next(); // Number
    ASSERT(tok->tt == token_type::NUMBER, "Not a number");
    
    const std::string& s = tok->content;
    
    if (s.length() > 1) {
        switch(s[1]) {
            case 'b': [[fallthrough]];
            case 'B': {
                u64 value{0};
                u8 charcount{0};
                
                for (u64 i = 2; i < s.length(); ++i) {
                    switch (s[i]) {
                        case '\'': [[fallthrough]];
                        case '_': continue;
                        case '0': [[fallthrough]];
                        case '1': 
                            ++charcount;
                            value = (value << 1) | ((s[i] - '0') & 1);
                            break;
                    }
                }
                
                // TODO Type choosing
                // TODO Check charcount
                
                return ast::make_value({value}, tok, tt.U64);
            }
            case 'o': [[fallthrough]];
            case 'O': {
                u64 value{0};
                u8 charcount{0};
                
                for (u64 i = 2; i < s.length(); ++i) {
                    switch (s[i]) {
                        case '\'': [[fallthrough]];
                        case '_': continue;
                        case '0': [[fallthrough]];
                        case '1': [[fallthrough]];
                        case '2': [[fallthrough]];
                        case '3': [[fallthrough]];
                        case '4': [[fallthrough]];
                        case '5': [[fallthrough]];
                        case '6': [[fallthrough]];
                        case '7':
                            ++charcount;
                            value = (value << 3) | ((s[i] - '0') & 7);
                            break;
                    }
                }
                
                // TODO Type choosing
                // TODO Check charcount
                
                return ast::make_value({value}, tok, tt.U64);
            }
            case 'x': [[fallthrough]];
            case 'X': {
                u64 value{0};
                u8 charcount{0};
                
                for (u64 i = 2; i < s.length(); ++i) {
                    switch (s[i]) {
                        case '\'': [[fallthrough]];
                        case '_': continue;
                        case '0': [[fallthrough]];
                        case '1': [[fallthrough]];
                        case '2': [[fallthrough]];
                        case '3': [[fallthrough]];
                        case '4': [[fallthrough]];
                        case '5': [[fallthrough]];
                        case '6': [[fallthrough]];
                        case '7': [[fallthrough]];
                        case '8': [[fallthrough]];
                        case '9':
                            ++charcount;
                            value = (value << 4) | ((s[i] - '0') & 0xF);
                            break;
                        case 'a': [[fallthrough]];
                        case 'b': [[fallthrough]];
                        case 'c': [[fallthrough]];
                        case 'd': [[fallthrough]];
                        case 'e': [[fallthrough]];
                        case 'f':
                            ++charcount;
                            value = (value << 4) | (((s[i] - 'a') + 10) & 0xF);
                            break;
                        case 'A': [[fallthrough]];
                        case 'B': [[fallthrough]];
                        case 'C': [[fallthrough]];
                        case 'D': [[fallthrough]];
                        case 'E': [[fallthrough]];
                        case 'F':
                            ++charcount;
                            value = (value << 4) | (((s[i] - 'A') + 10) & 0xF);
                            break;
                    }
                }
                
                // TODO Type choosing
                // TODO Check charcount
                
                return ast::make_value({value}, tok, tt.U64);
            }
            default:
                break;
        }
    }
    
    // Base 10
    __uint128_t integer{0};
    __uint128_t frac{0};
    u8 frac_digits{0};
    s16 exp{0};
    bool has_exp{false};
    s8 exp_mul{0};
    bool is_double{false};
    bool is_float{false};
    
    for (const char& c : s) {
        
        switch(c) {
            case '\'': [[fallthrough]];
            case '_': continue;
            case 'E': [[fallthrough]];
            case 'e':
                if (has_exp) {
                    // error
                } else {
                    has_exp = true;
                }
                break;
            case '+':
                if (!has_exp || exp_mul || exp) {
                    // error
                } else {
                    exp_mul = 1;
                }
                break;
            case '-': 
                if (!has_exp || exp_mul || exp) {
                    // error
                } else {
                    exp_mul = -1;
                }
                break;
            case '.': 
                if (is_double) {
                    // error
                } else {
                    is_double = true;
                }
                break;
            case 'F': [[fallthrough]];
            case 'f': 
                is_float = true;
                if (&c != &s[s.length() - 1]) {
                    // error;
                }
                break;
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
                if (has_exp) {
                    exp *= 10;
                    exp += c - '0';
                } else if (is_double) {
                    frac *= 10;
                    frac += c - '0';
                    frac_digits++;
                } else {
                    integer *= 10;
                    integer += c - '0';
                }
            }
            default:
                // error
                continue;
        }
    }
    
    union {
        u64 value;
        double d;
        float f;
    } punner;
    type* t{nullptr};
    
    // NOTE: Very, veeeery naive implementation. I'll get back to this
    
    if (is_double) {
        double d{0};
        d += (double) integer;
        d += ((double) frac / (double) frac_digits);
        d *= std::pow(10, exp * (exp_mul ? exp_mul : 1));
        punner.d = d;
        t = tt.F64;
    } else if (is_float) {
        float f{0};
        f += (float) integer;
        f += ((float) frac / (float) frac_digits);
        f *= std::pow(10, exp * (exp_mul ? exp_mul : 1));
        punner.f = f;
        t = tt.F32;
    } else {
        integer *= std::pow(10, exp * (exp_mul ? exp_mul : 1));
        // TODO Check that integer and frac are not too large
        u64 value{(u64) integer};
        punner.value = value;
        // TODO Check type for numbers
        t = tt.U64;
    }
    
    return ast::make_value({punner.value}, tok, t);
}

ast* pass::identifier() {
    token* tok = next(); // Identifier
    ASSERT(tok->tt == token_type::IDENTIFIER, "Not an identifier");
    
    return ast::make_iden({nullptr}, tok, tt.NONE);
}

ast* pass::char_lit() {
    token* tok = next(); // Character literal
    ASSERT(tok->tt == token_type::CHARACTER, "Not a character");
    const std::string& s = tok->content;
    
    if (s.length() < 4 || s[1] != '\'' || s[s.length() - 1] != '\'') {
        // error
        ast* ret = ast::make_value({0}, tok, tt.C32);
        mod.errors.push_back({ret, "Invalid char literal"});
        return ret;
    }
    
    u8 plen{0};
    u64 res = utf8_to_utf32(s.c_str() + 2, &plen);
    if (res > 0x10FFF || plen != s.length() - 5) {
        ast* ret = ast::make_value({0}, tok, tt.C32);
        mod.errors.push_back({ret, "Invalid char literal"});
        return ret;
    }
    u32 value{(u32) res};
    
    return ast::make_value({value}, tok, tt.C32);
}

ast* pass::string_lit() {
    // TODO Interning!
    
    token* tok = next(); // String literal
    ASSERT(tok->tt == token_type::STRING, "Not a string");
    const std::string& s = tok->content;
    
    u64 start = 0, end = s.length() - 1;
    
    if (s[end] != '"') {
        ast* ret = ast::make_string(ast_string{""}, tok, tt.sized_array_of(tt.C8, 0, true));
        mod.errors.push_back({ret, "Invalid char literal"});
        return ret;
    } else if (s[start] == '\'') {
        for (u64 i = start; i < end; ++i) {
            if (s[i] == '"') {
                start = i;
                break;
            }
        }
    } else if (s[start] != '"') {
        ast* ret = ast::make_string(ast_string{""}, tok, tt.sized_array_of(tt.C8, 0, true));
        mod.errors.push_back({ret, "Invalid char literal"});
        return ret;
    }
    
    
    enum class string_flags {
        utf8, utf16, utf32, c_str
    } sflags{string_flags::utf8};
    bool warn{false};
    
    if (start != 0) {
        using namespace std::string_literals;
        std::string flags = s.substr(1, start);
        if (flags == "c"s) {
            sflags = string_flags::c_str;
        } else if (flags == "u8"s) {
            sflags = string_flags::utf8;
        } else if (flags == "u16"s) {
            sflags = string_flags::utf16;
        } else if (flags == "u32"s) {
            sflags = string_flags::utf32;
        } else {
            sflags = string_flags::utf32;
            warn = true;
        }
    }
    
    // Please... my buffer overflows... they are very sick...
    u8* buff = new u8[32];
    u8 buff_size = 32;
    u8 rlen{0};
    u64 i = start + 1;
    const char* c = s.c_str();
    while (i < end && c[i]) {
        u8 len{0};
        u8 plen{0};
        u64 res = read_utf8(c + i, len, &plen);
        if (res > 0xFFFFFFFF) {
            delete [] buff;
            ast* ret = ast::make_string(ast_string{""}, tok, tt.sized_array_of(tt.C8, 0, true));
            mod.errors.push_back({ret, "Invalid char literal"});
            return ret;
        }
        if (rlen + plen > buff_size) {
            u8* buffbuff = new u8[buff_size * 2];
            std::memcpy(buffbuff, buff, rlen);
            delete [] buff;
            buff = buffbuff;
            buff_size *= 2;
        }
        std::memcpy(buff + rlen, &res, len);
        rlen += len;
        i += plen;
    }
    
    if (i != end || c[i] != '"') {
        delete [] buff;
        ast* ret = ast::make_string(ast_string{""}, tok, tt.sized_array_of(tt.C8, 0, true));
        mod.errors.push_back({ret, "Invalid char literal"});
        return ret;
    }
    
    if (sflags == string_flags::utf8) {
        return ast::make_string(ast_string{buff, rlen}, tok, tt.sized_array_of(tt.C8, rlen, true));
    } else {
        // TODO
        ASSERT(false, "Murder this");
        return nullptr;
    }
        
}

ast* pass::note() {
    token* tok = next(); // Note
    ASSERT(tok->tt == token_type::COMPILER, "Not a compiler note");
    
    // TODO Actually use notes we need
    if (is_symbol(grammar::OPAREN)) {
        while (is_symbol(grammar::COMMA) || is_symbol(grammar::OPAREN)) {
            next(); // ( or ,
            expression();
        }
        if (require_symbol(grammar::CPAREN)) next(); // )
    }
    
    
    return ast::make_iden({nullptr}, tok, tt.NONE);
}

ast* pass::array_lit() {
    ASSERT(is_symbol(grammar::LITERAL_ARRAY), "Invalid start to array literal");
    
    ast* ret = ast::make_compound({}, c, tt.NONE_ARRAY);
    
    while (is_symbol(grammar::COMMA) || is_symbol(grammar::LITERAL_ARRAY)) {
        next(); // '[ ,
        ast* e = expression();
        ret->compound.elems.push_back(e);
    }
    
    if (require_symbol(grammar::CBRACK)) next(); // ]
    
    return ret;
}

ast* pass::struct_lit() {
    ASSERT(is_symbol(grammar::LITERAL_STRUCT), "Invalid start to struct literal");
    
    ast* ret = ast::make_compound({}, c, tt.NONE_STRUCT);
    
    while (is_symbol(grammar::COMMA) || is_symbol(grammar::LITERAL_STRUCT)) {
        next(); // '{ ,
        ast* e = expression();
        ret->compound.elems.push_back(ast::make_binary({grammar::SP_NAMED, nullptr, e}, e->tok, e->t));
    }
    
    if (is_symbol(grammar::ASSIGN)) {
        ast* iden = ret->compound.elems.tail;
        ASSERT(iden && iden->tt == ast_type::BINARY && iden->binary.sym == grammar::SP_NAMED, "Invalid struct element");
        next(); // =
        iden->binary.left = expression();
        while (is_symbol(grammar::COMMA)) {
            next(); // ,
            ast* n = expression(); // iden?
            if (require_symbol(grammar::ASSIGN)) next(); // =
            ast* e = expression();
            ret->compound.elems.push_back(ast::make_binary({grammar::SP_NAMED, n, e}, e->tok, e->t));
        }
    }
    
    if (require_symbol(grammar::CBRACE)) next(); // }
    
    return ret;
}

ast* pass::tuple_lit() {
    ASSERT(is_symbol(grammar::LITERAL_TUPLE), "Invalid start to tuple literal");
    
    ast* ret = ast::make_compound({}, c, tt.NONE_TUPLE);
    
    while (is_symbol(grammar::COMMA) || is_symbol(grammar::LITERAL_TUPLE)) {
        next(); // '( ,
        ast* e = expression();
        ret->compound.elems.push_back(e);
    }
    
    if (require_symbol(grammar::CPAREN)) next(); // )
    
    return ret;
}


ast* pass::program_unit() {
    return nullptr;
}

ast* pass::freestmt() {
    return nullptr;
}

ast* pass::stmt() {
    return nullptr;
}

ast* pass::scopestmt() {
    return nullptr;
}


ast* pass::scope() {
    return nullptr;
}


ast* pass::optscope() {
    return nullptr;
}


ast* pass::optvardecls() {
    return nullptr;
}


ast* pass::ifstmt() {
    return nullptr;
}

ast* pass::ifscope() {
    return nullptr;
}


ast* pass::forstmt() {
    return nullptr;
}

ast* pass::whilestmt() {
    return nullptr;
}

ast* pass::whilecond() {
    return nullptr;
}

ast* pass::whiledostmt() {
    return nullptr;
}

ast* pass::dowhilestmt() {
    return nullptr;
}


ast* pass::switchstmt() {
    return nullptr;
}

ast* pass::switchscope() {
    return nullptr;
}

ast* pass::casedecl() {
    return nullptr;
}


ast* pass::trystmt() {
    return nullptr;
}


ast* pass::returnstmt() {
    return nullptr;
}

ast* pass::raisestmt() {
    return nullptr;
}


ast* pass::gotostmt() {
    return nullptr;
}

ast* pass::labelstmt() {
    return nullptr;
}


ast* pass::deferstmt() {
    return nullptr;
}

ast* pass::breakstmt() {
    return nullptr;
}

ast* pass::continuestmt() {
    return nullptr;
}


ast* pass::importstmt() {
    return nullptr;
}

ast* pass::usingstmt() {
    return nullptr;
}


ast* pass::namespacestmt() {
    return nullptr;
}

ast* pass::namespacescope() {
    return nullptr;
}


ast* pass::declarator() {
    return nullptr;
}

ast* pass::_type() {
    return nullptr;
}

ast* pass::inferrable_type() {
    return nullptr;
}


ast* pass::paramtype() {
    return nullptr;
}

ast* pass::rettype() {
    return nullptr;
}


ast* pass::declstmt() {
    return nullptr;
}

ast* pass::defstmt() {
    return nullptr;
}


ast* pass::vardecl() {
    return nullptr;
}

ast* pass::simplevardecl() {
    return nullptr;
}


ast* pass::funclit_or_type() {
    return nullptr;
}

ast* pass::funclitdef() {
    return nullptr;
}

ast* pass::functypesig() {
    return nullptr;
}

ast* pass::funcparam() {
    return nullptr;
}

ast* pass::funcret() {
    return nullptr;
}


ast* pass::structtypelit() {
    return nullptr;
}

ast* pass::structtypelitdef() {
    return nullptr;
}

ast* pass::structscope() {
    return nullptr;
}

ast* pass::structvardecl() {
    return nullptr;
}


ast* pass::uniontypelit() {
    return nullptr;
}

ast* pass::uniontypelitdef() {
    return nullptr;
}


ast* pass::enumtypelit() {
    return nullptr;
}

ast* pass::enumtypelitdef() {
    return nullptr;
}

ast* pass::enumscope() {
    return nullptr;
}


ast* pass::tupletypelit() {
    return nullptr;
}

ast* pass::tupletypelitdef() {
    return nullptr;
}

ast* pass::tupletypes() {
    return nullptr;
}


ast* pass::typelit() {
    return nullptr;
}

ast* pass::typelit_nofunc() {
    return nullptr;
}


ast* pass::assignment() {
    return nullptr;
}

ast* pass::assstmt() {
    return nullptr;
}


ast* pass::deletestmt() {
    return nullptr;
}


ast* pass::expressionstmt() {
    return nullptr;
}

ast* pass::expression() {
    return nullptr;
}

ast* pass::ternaryexpr() {
    return nullptr;
}

ast* pass::newexpr() {
    return nullptr;
}


ast* pass::prefixexpr() {
    return nullptr;
}

ast* pass::postfixexpr() {
    return nullptr;
}

ast* pass::infixexpr() {
    return nullptr;
}

ast* pass::postcircumfixexpr() {
    return nullptr;
}

ast* pass::function_call() {
    return nullptr;
}

ast* pass::access() {
    return nullptr;
}

ast* pass::reorder() {
    return nullptr;
}

ast* pass::dotexpr() {
    return nullptr;
}


ast* pass::literal() {
    return nullptr;
}

ast* pass::literalexpr() {
    return nullptr;
}

ast* pass::identifierexpr() {
    return nullptr;
}

ast* pass::parenexpr() {
    return nullptr;
}


ast* pass::select() {
    return nullptr;
}

ast* pass::compound_identifier_simple() {
    return nullptr;
}

ast* pass::make_error_ast(token* t) {
    errors.push_back(ast::make_none({}, t, nullptr));
    return errors.tail;
}
