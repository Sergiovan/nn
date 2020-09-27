#include "frontend/compilers/file_parser.h"

#include <cmath>
#include <cstring>
#include <filesystem>
#include "common/util.h"
#include "common/logger.h"

using pass = file_parser;

pass::file_parser(compiler& p, nnmodule& mod) : comp{p}, mod{mod}, tt{mod.tt} {
    using namespace grammar;
    
    constexpr std::array<grammar::symbol, 16> pre {{
        SPREAD, INCREMENT, DECREMENT, SUB, INFO, NOT, LNOT, AT, POINTER, WEAK_PTR,
        OBRACK, ADD, KW_TYPEOF, KW_SIZEOF, KW_CONST, KW_VOLAT
    }};
    
    constexpr std::array<grammar::symbol, 2> post {{
        INCREMENT, DECREMENT
    }};
    
    constexpr std::array<grammar::symbol, 27> infix {{
        KW_AS, MUL, DIV, IDIV, MODULO, ADD, SUB, CONCAT, 
        SHL, SHR, RTL, RTR, BITSET, BITCLEAR, BITTOGGLE, BITCHECK,
        AND, OR, XOR, LT, LE, GT, GE, EQUALS, NEQUALS, LAND, LOR
    }};
    
//     constexpr std::array<grammar::symbol, 3> post_circumfix {{
//         OPAREN, OBRACK, OSELECT
//     }};
    
    for (auto e : pre) {
        pre_ops.insert(e);
    }
    
    for (auto e : post) {
        post_ops.insert(e);
    }
    
    for (auto e : infix) {
        infix_ops.insert(e);
    }
    
}

bool pass::read() {
    mod.ts.read();
    
    if (mod.ts.head->tt == token_type::ERROR) {
        mod.errors.push_back({make_error_ast(mod.ts.head), mod.ts.head->content});
        return false;
    } else {
        return true;
    }
}

bool pass::pass() {    
    n = mod.ts.head;
    clear();
    c = n;
    next();
    
    mod.root = program_unit();
    return !mod.errors.size();
}

token* pass::next() {
    return next(grammar::SPECIAL_INVALID);
}

token* pass::next(grammar::symbol expected) {
    using grammar::string_to_symbol;
    
    token* buff = c;
    c = peek(expected);
    return buff;
}

token* pass::peek(grammar::symbol expected) {
    using grammar::string_to_symbol;
    
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
            case token_type::END_OF_FILE: {
                n = buff; // Go back
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
            case token_type::END_OF_FILE: [[fallthrough]];
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

bool pass::is_assign() {
    return c->tt == token_type::SYMBOL && (c->value >= grammar::ASSIGN && c->value <= grammar::MODULO_ASSIGN);
}

bool pass::require_assign() {
    if (is_assign()) {
        return true;
    } else {
        mod.errors.push_back({make_error_ast(c), "Expecting assignment"});
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
    if (res > 0x10FFF || plen != s.length() - 3) {
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
        mod.errors.push_back({ret, "Invalid string literal"});
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
        mod.errors.push_back({ret, "Invalid string literal"});
        return ret;
    }
    
    
    enum class string_flags {
        utf8, utf16, utf32, c_str
    } sflags{string_flags::utf8};
    [[maybe_unused]]
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
            mod.errors.push_back({ret, "Invalid string literal"});
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
        mod.errors.push_back({ret, "Invalid string literal"});
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
            expression(); // TODO Stop bleeding memory holy fuck
        }
        // TODO err
        if (require_symbol(grammar::CPAREN)) next(); // )
    }
    
    
    return ast::make_iden({nullptr}, tok, tt.NONE);
}

ast* pass::array_lit() {
    ASSERT(is_symbol(grammar::LITERAL_ARRAY), "Invalid start to array literal");
    
    ast* ret = ast::make_compound({}, c, tt.NONE_ARRAY);
    next(); // '[
    
    if (!is_symbol(grammar::CBRACK)) {
        do {
            ast* e = expression();
            ret->compound.elems.push_back(e);
        } while (is_symbol(grammar::COMMA) && next());
    }
    
    // TODO err
    if (require_symbol(grammar::CBRACK)) next(); // ]
    
    return ret;
}

ast* pass::struct_lit() {
    ASSERT(is_symbol(grammar::LITERAL_STRUCT), "Invalid start to struct literal");
    
    ast* ret = ast::make_compound({}, c, tt.NONE_STRUCT);
    next(); // '{
    
    if (!is_symbol(grammar::CBRACE)) {
        do {
            ast* e = expression();
            ret->compound.elems.push_back(ast::make_binary({grammar::SP_NAMED, ast::make_none({}, e->tok, tt.TYPELESS), e}, e->tok, e->t));
        } while (is_symbol(grammar::COMMA) && next());
    
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
    }
    
    // TODO err
    if (require_symbol(grammar::CBRACE)) next(); // }
    
    return ret;
}

ast* pass::tuple_lit() {
    ASSERT(is_symbol(grammar::LITERAL_TUPLE), "Invalid start to tuple literal");
    
    ast* ret = ast::make_compound({}, c, tt.NONE_TUPLE);
    next(); // '(
        
    if (!is_symbol(grammar::CPAREN)) {
        do {
            ast* e = expression();
            ret->compound.elems.push_back(e);
        } while (is_symbol(grammar::COMMA) && next());
    }
    
    // TODO err
    if (require_symbol(grammar::CPAREN)) next(); // )
    
    return ret;
}


ast* pass::program_unit() {
    ast* ret = ast::make_block({}, c, tt.TYPELESS);
    
    while (!is(token_type::END_OF_FILE)) {
        ret->block.elems.push_back(freestmt());
    }
    
    return ret;
}

ast* pass::freestmt() {
    while (is(token_type::COMPILER)) {
        note(); // Ignore for now
    }
    
    require(token_type::KEYWORD); // ???
    
    switch (c->value) {
        case grammar::KW_USING:
            return usingstmt();
        case grammar::KW_IMPORT:
            return importstmt();
        case grammar::KW_NAMESPACE:
            return namespacestmt();
        case grammar::KW_DEF: [[fallthrough]];
        case grammar::KW_VAR: [[fallthrough]];
        case grammar::KW_LET: [[fallthrough]];
        case grammar::KW_REF:
            return declstmt();
        default:
            // TODO err
            ast* ret = make_error_ast(c);
            next(); // ???
            return ret;
    }
}

ast* pass::stmt() {
    while (is(token_type::COMPILER)) {
        note(); // Ignore for now
    }
    
    if (is(token_type::KEYWORD)) {
        switch (c->value) {
            case grammar::KW_IF:
                return ifstmt();
            case grammar::KW_FOR:
                return forstmt();
            case grammar::KW_WHILE: [[fallthrough]];
            case grammar::KW_LOOP:
                return whilestmt();
            case grammar::KW_SWITCH:
                return switchstmt();
            case grammar::KW_TRY:
                return trystmt();
            case grammar::KW_RETURN:
                return returnstmt();
            case grammar::KW_RAISE:
                return raisestmt();
            case grammar::KW_GOTO:
                return gotostmt();
            case grammar::KW_LABEL:
                return labelstmt();
            case grammar::KW_DEFER:
                return deferstmt();
            case grammar::KW_BREAK:
                return breakstmt();
            case grammar::KW_CONTINUE:
                return continuestmt();
            case grammar::KW_USING:
                return usingstmt();
            case grammar::KW_NAMESPACE:
                return namespacestmt();
            case grammar::KW_DELETE:
                return deletestmt();
            case grammar::KW_DEF: [[fallthrough]];
            case grammar::KW_VAR: [[fallthrough]];
            case grammar::KW_LET: [[fallthrough]];
            case grammar::KW_REF:
                return declstmt();
            default:
                return assorexpr();
        }
    } else if (is(token_type::SYMBOL)) {
        switch (c->value) {
            case grammar::OBRACE:
                return scope();
            case grammar::SEMICOLON: {
                token* tok = next(); // ;
                return ast::make_none({}, tok, tt.TYPELESS);
            }
            default:
                return assorexpr();
        }
    } else {
        return assorexpr();
    }
}

ast* pass::scopestmt() {
    while (is(token_type::COMPILER)) {
        note(); // Ignore for now
    }
    
    if (is(token_type::KEYWORD)) {
        switch (c->value) {
            case grammar::KW_IF:
                return ifstmt();
            case grammar::KW_FOR:
                return forstmt();
            case grammar::KW_WHILE: [[fallthrough]];
            case grammar::KW_LOOP:
                return whilestmt();
            case grammar::KW_SWITCH:
                return switchstmt();
            case grammar::KW_RETURN:
                return returnstmt();
            case grammar::KW_RAISE:
                return raisestmt();
            case grammar::KW_GOTO:
                return gotostmt();
            case grammar::KW_BREAK:
                return breakstmt();
            case grammar::KW_CONTINUE:
                return continuestmt();
            case grammar::KW_DELETE:
                return deletestmt();
            default:
                return assorexpr();
        }
    } else if (is(token_type::SYMBOL)) {
        switch (c->value) {
            case grammar::OBRACE:
                return scope();
            case grammar::SEMICOLON: {
                token* tok = next(); // ;
                return ast::make_none({}, tok, tt.TYPELESS);
            }
            default:
                return assorexpr();
        }
    } else {
        return assorexpr();
    }
}


ast* pass::scope() {
    ASSERT(is_symbol(grammar::OBRACE), "Start of scope was not {");
    token* s = next(); // {
    
    ast* ret = ast::make_block({}, s, tt.TYPELESS);
    
    while (!is_symbol(grammar::CBRACE) && !is(token_type::END_OF_FILE)) {
        ret->block.elems.push_back(stmt());
    }
    
    // TODO err
    if (require_symbol(grammar::CBRACE)) next(); // }
    
    return ret;
}


ast* pass::optscope() {
    if (is_keyword(grammar::KW_DO)) {
        next(); // do
        return scopestmt();
    } else if (is_symbol(grammar::OBRACE)) {
        return scope();
    } else {
        // TODO err
        ast* ret = make_error_ast(c);
        next(); // ???
        mod.errors.push_back({ret, "Unexpected symbol"});
        return ret;
    }
}


ast* pass::optvardecls() {
    ast* ret = ast::make_block({}, c, tt.TYPELESS);
    
    while (is_keyword(grammar::KW_LET) || is_keyword(grammar::KW_VAR) || is_keyword(grammar::KW_REF)) {
        ret->block.elems.push_back(vardecl());
        
        // TODO err
        if (require_symbol(grammar::SEMICOLON)) next(); // ;
    }
    
    return ret;
}


ast* pass::ifstmt() {
    ASSERT(is_keyword(grammar::KW_IF), "Expected 'if'");
    
    token* tok = next(); // if
    
    ast* ret = ast::make_binary({grammar::KW_IF}, tok, tt.TYPELESS);
    
    ast* left = ret->binary.left = ast::make_block({}, c, tt.TYPELESS);
    left->block.elems.push_back(optvardecls());
    left->block.elems.push_back(expression());
    
    ast* right = ret->binary.right = ast::make_binary({grammar::KW_ELSE}, tok, tt.TYPELESS);
    if (is_keyword(grammar::KW_DO)) {
        next(); // do
        right->binary.left = scopestmt();
        right->binary.right = ast::make_none({}, tok, tt.TYPELESS);
    } else if (is_symbol(grammar::OBRACE)) {
        right->binary.left = scopestmt();
        
        if (is_keyword(grammar::KW_ELSE)) {
            next(); // else
            right->binary.right = scopestmt();
        } else {
            right->binary.right = ast::make_none({}, tok, tt.TYPELESS);
        }
    } else {
        // TODO err
        ast* res = make_error_ast(c);
        next(); // ???
        right->binary.left = res; // ???????
        right->binary.right = ast::make_none({}, tok, tt.TYPELESS);
        mod.errors.push_back({res, "Unexpected symbol"});
    }
    
    return ret;
}


ast* pass::forstmt() {
    ASSERT(is_keyword(grammar::KW_FOR), "Expected 'for'");
    token* tok = next(); // for
    
    ast* ret = ast::make_binary({grammar::KW_FOR}, tok, tt.TYPELESS);
    ast* decl{nullptr};
    if (is_keyword(grammar::KW_LET) || is_keyword(grammar::KW_REF) || require_keyword(grammar::KW_VAR)) {
        decl = vardecl(true);
    } else {
        // TODO err
        decl = make_error_ast(c);
        
    }
    token* in = c;
    
    // TODO err
    if (require_keyword(grammar::KW_IN)) next(); // in
    
    ast* value = expression();
    ret->binary.left = ast::make_binary({grammar::KW_IN, decl, value}, in, tt.TYPELESS);
    ret->binary.right = optscope();
    
    return ret;
}

ast* pass::whilestmt() {
    if (is_keyword(grammar::KW_WHILE)) {
        return whiledostmt();
    } else if (is_keyword(grammar::KW_LOOP)) {
        return dowhilestmt();
    } else {
        // TODO err
        ASSERT(false, "Expected 'while' or 'loop'");
        return ast::make_none({}, c, tt.TYPELESS); // Unreachable
    }
}

ast* pass::whilecond() {
    ast* ret = ast::make_block({}, c, tt.TYPELESS);
    
    ret->block.elems.push_back(expression());
        
    while (is_symbol(grammar::SEMICOLON)) {
        next(); // ;
        ret->block.elems.push_back(assorexpr(false)); // Do not consume the semicolons please ;-;
    }
    
    return ret;
}

ast* pass::whiledostmt() {
    ASSERT(is_keyword(grammar::KW_WHILE), "Expected 'while'");
    token* tok = next(); // while
    
    ast* res = ast::make_binary({grammar::KW_WHILE}, tok, tt.TYPELESS);
    ast* left = res->binary.left = ast::make_binary({grammar::SP_COND}, tok, tt.TYPELESS);
    left->binary.left = optvardecls();
    left->binary.right = whilecond();
    
    res->binary.right = optscope();
    
    return res;
}

ast* pass::dowhilestmt() {
    ASSERT(is_keyword(grammar::KW_LOOP), "Expected 'loop'");
    token* tok = next(); // while
    
    ast* res = ast::make_binary({grammar::KW_LOOP}, tok, tt.TYPELESS);
    
    ast* left = res->binary.left = ast::make_binary({grammar::SP_COND}, tok, tt.TYPELESS);
    left->binary.left = optvardecls();
    
    res->binary.right = optscope();
    
    // TODO err
    if (require_keyword(grammar::KW_WHILE)) next(); // while
    
    left->binary.right = assorexpr();
    
    return res;
}


ast* pass::switchstmt() {
    ASSERT(is_keyword(grammar::KW_SWITCH), "Expected 'switch'");
    
    token* tok = next(); // switch
    ast* ret = ast::make_binary({grammar::KW_SWITCH}, tok, tt.TYPELESS);
    ret->binary.left = optvardecls();
    ASSERT(ret->binary.left->is_block(), "Result of optvardecls() wasn't a block");
    ret->binary.left->block.elems.push_back(expression());
    
    if (require_symbol(grammar::OBRACE)) {
        ret->binary.right = switchscope();
    } else {
        // TODO err
        next(); // ???
    }
    
    return ret;
}

ast* pass::switchscope() {
    ASSERT(is_symbol(grammar::OBRACE), "Expected '{'");
    
    token* tok = next(); // {
    
    ast* ret = ast::make_block({}, tok, tt.TYPELESS);
    
    while (!is_symbol(grammar::CBRACE) && !is(token_type::END_OF_FILE)) {
        while (is(token_type::COMPILER)) {
            note(); // Ignore for now
        }
        
        ret->block.elems.push_back(casedecl());
    }
    
    // TODO err
    if (require_symbol(grammar::CBRACE)) next(); // }
    
    return ret;
}

ast* pass::casedecl() {
    ast* ret = ast::make_binary({}, c, tt.TYPELESS);
    
    if (is_keyword(grammar::KW_CASE)) {
        ret->binary.sym = grammar::KW_CASE;
        ret->tok = next();
        
        ast* left = ret->binary.left = ast::make_block({}, c, tt.TYPELESS);
        left->block.elems.push_back(expression());
        while (is_symbol(grammar::COMMA)) {
            next(); // ,
            left->block.elems.push_back(expression());
        }
        
        if (is_keyword(grammar::KW_CONTINUE)) {
            token* tok = next(); // continue
            ret->binary.right = ast::make_zero({grammar::KW_CONTINUE}, tok, tt.TYPELESS);
            
            if (require_symbol(grammar::SEMICOLON)) next(); // ;
        } else {
            ret->binary.right = optscope();
        }
        
        
    } else if (is_keyword(grammar::KW_ELSE)) {
        ret->binary.sym = grammar::KW_ELSE;
        ret->tok = next();
        ret->binary.left = ast::make_none({}, c, tt.TYPELESS);
        
        if (is_keyword(grammar::KW_CONTINUE)) {
            token* tok = next();
            ret->binary.right = ast::make_zero({grammar::KW_CONTINUE}, tok, tt.TYPELESS);
        } else {
            ret->binary.right = optscope();
        }
        
    } else {
        // TODO err
        ret->binary.sym = grammar::SPECIAL_INVALID;
        ret->binary.left = ast::make_none({}, c, tt.TYPELESS);
        ret->binary.right = ast::make_none({}, c, tt.TYPELESS);
        next(); // ???
        mod.errors.push_back({ret, "Invalid switch case"});
    }
    
    return ret;
}


ast* pass::trystmt() {
    ASSERT(is_keyword(grammar::KW_TRY), "Expected 'try'");
    token* tok = next(); // try
    
    ast* ret = ast::make_binary({grammar::KW_TRY}, tok, tt.TYPELESS);
    ast* left = ret->binary.left = ast::make_block({}, tok, tt.TYPELESS);
    
    while (!is_keyword(grammar::KW_CATCH) && !is_keyword(grammar::KW_RAISE) && 
           !is(token_type::END_OF_FILE)) {
        left->block.elems.push_back(stmt());
    }
    
    if (is_keyword(grammar::KW_CATCH)) {
        token* tok = next();
        ast* right = ret->binary.right = ast::make_binary({grammar::KW_CATCH}, tok, tt.TYPELESS);
        
        if (is_keyword(grammar::KW_E64)) {
            next(); // e64
            if (require(token_type::IDENTIFIER)) { 
                right->binary.left = identifier();
            } else {
                right->binary.left = ast::make_none({}, c, tt.TYPELESS);
            }
        }
        
        if (require_symbol(grammar::OBRACE)) {
            right->binary.right = switchscope();
        } else {
            // TODO err
            right->binary.right = ast::make_none({}, c, tt.TYPELESS);
            next(); // ???
        }
    } else if (is_keyword(grammar::KW_RAISE)) {
        
        ret->binary.right = ast::make_zero({grammar::KW_RAISE}, tok, tt.TYPELESS);
        
        next(); // raise
        if (require_symbol(grammar::SEMICOLON)) next();
        
        
    } else {
        // TODO err
        ret->binary.right = ast::make_none({}, c, tt.TYPELESS);
        next(); // ???
        mod.errors.push_back({ret, "Invalid catch case"});
    }
    
    return ret;
}


ast* pass::returnstmt() {
    ASSERT(is_keyword(grammar::KW_RETURN), "Expected 'return'");
    token* tok = next(); // return
    
    ast* ret = ast::make_unary({grammar::KW_RETURN, ast::make_block({}, tok, tt.TYPELESS)}, tok, tt.TYPELESS);
    if (is_keyword(grammar::KW_U0)) {
        next(); // u0
    } else if (!is_symbol(grammar::SEMICOLON)) {
        ret->unary.node->block.elems.push_back(expression());
        
        while (is_symbol(grammar::COMMA)) {
            next(); // ,
            ret->unary.node->block.elems.push_back(expression());
        }
    }
    
    // TODO err
    if (require_symbol(grammar::SEMICOLON)) next(); // ;
    
    return ret;
}

ast* pass::raisestmt() {
    ASSERT(is_keyword(grammar::KW_RAISE), "Expected 'raise'");
    token* tok = next(); // raise
    
    ast* ret = ast::make_unary({grammar::KW_RAISE, ast::make_block({}, tok, tt.TYPELESS)}, tok, tt.TYPELESS);
    ret->unary.node->block.elems.push_back(expression());
    
    if (!is_symbol(grammar::SEMICOLON)) {
        ret->unary.node->block.elems.push_back(expression()); // Constant expression?
    }
    
    // TODO err
    if (require_symbol(grammar::SEMICOLON)) next(); // ;
    
    return ret;
}


ast* pass::gotostmt() {
    ASSERT(is_keyword(grammar::KW_GOTO), "Expected 'goto'");
    token* tok = next(); // goto
    
    ast* ret = ast::make_unary({grammar::KW_GOTO}, tok, tt.TYPELESS);
    if (require(token_type::IDENTIFIER)) {
        ret->unary.node = identifier();
    } else {
        ret->unary.node = ast::make_none({}, tok, tt.TYPELESS);
    }
    
    // TODO err
    if (require_symbol(grammar::SEMICOLON)) next(); // ;
    
    return ret;
}

ast* pass::labelstmt() {
    ASSERT(is_keyword(grammar::KW_LABEL), "Expected 'label'");
    token* tok = next(); // label
    
    ast* ret = ast::make_unary({grammar::KW_LABEL}, tok, tt.TYPELESS);
    if (require(token_type::IDENTIFIER)) {
        ret->unary.node = identifier();
    } else {
        ret->unary.node = ast::make_none({}, tok, tt.TYPELESS);
    }
    
    // TODO err
    if (require_symbol(grammar::SEMICOLON)) next(); // ;
    
    return ret;
}


ast* pass::deferstmt() {
    ASSERT(is_keyword(grammar::KW_DEFER), "Expected 'defer'");
    token* tok = next(); // defer
    
    ast* ret = ast::make_unary({grammar::KW_DEFER}, tok, tt.TYPELESS);
    ret->unary.node = scopestmt();
    
    return ret;
}

ast* pass::breakstmt() {
    ASSERT(is_keyword(grammar::KW_BREAK), "Expected 'break'");
    token* tok = next(); // break
    
    ast* ret = ast::make_zero({grammar::KW_LABEL}, tok, tt.TYPELESS);
    
    // TODO err
    if (require_symbol(grammar::SEMICOLON)) next(); // ;
    
    return ret;
}

ast* pass::continuestmt() {
    ASSERT(is_keyword(grammar::KW_CONTINUE), "Expected 'continue'");
    token* tok = next(); // continue
    
    ast* ret = ast::make_zero({grammar::KW_CONTINUE}, tok, tt.TYPELESS);
    
    // TODO err
    if (require_symbol(grammar::SEMICOLON)) next(); // ;
    
    return ret;
}


ast* pass::importstmt() {
    ASSERT(is_keyword(grammar::KW_IMPORT), "Expected 'import'");
    token* tok = next(); // import
    
    ast* ret = ast::make_binary({grammar::KW_IMPORT}, tok, tt.TYPELESS);
    
    namespace fs = std::filesystem;
    
    fs::path imprt{};
    
    if (is(token_type::STRING)) {
        ast* str = ret->binary.left = string_lit();
        
        ASSERT(str->tt == ast_type::STRING, "string_lit() returned non-string");
        
        imprt = std::string{str->string.chars, str->string.chars + str->string.length};
        
    } else {
        ast* un = ret->binary.left = compound_identifier_simple();
        
        ASSERT(un->tt == ast_type::UNARY, "compound_identifier_simple() returned non-unary");
        ASSERT(un->unary.node->tt == ast_type::BLOCK, "compound_identifier_simple() unary node was not a block");
        
        fs::path cf{mod.ts.get_name()};
        imprt = cf.parent_path();
        
        for (auto c : un->unary.node->block.elems) {
            ASSERT(c->tt == ast_type::IDENTIFIER, "compound_identifier_simple() was not made up of identifiers");
            imprt /= c->tok->content; // Hmmmm
        }
        
        imprt.replace_extension("nn");
        
    }
    
    logger::debug() << "Imported path was " << imprt;
    
    // TODO Path search shenanigans
    
    if (fs::exists(imprt)) {
        std::string abs = fs::absolute(imprt);
        nnmodule* m = comp.get(abs);
        if (!m) {
            comp.parse_file_task(abs, &mod);
        } else if (m == &mod) {
            mod.errors.push_back({ret, ss::get() << "Cannot import self" << ss::end()});
        }
    } else {
        // TODO err
        mod.errors.push_back({ret, ss::get() << "File " << imprt << " could not be found" << ss::end()});
    }
    
    if (is_symbol(grammar::KW_AS)) {
        next(); // as
        
        if (is(token_type::IDENTIFIER)) {
            ret->binary.right = identifier();
        } else {
            // TODO err
            ret->binary.right = ast::make_none({}, c, tt.TYPELESS);
            if (!is_symbol(grammar::SEMICOLON)) {
                next(); // ???
            }
            mod.errors.push_back({ret, "Invalid identifier"});
        }
    } else {
        ret->binary.right = ast::make_none({}, c, tt.TYPELESS);
    }
    
    // TODO err
    if (require_symbol(grammar::SEMICOLON)) next(); // ;
    
    return ret;
}

ast* pass::usingstmt() {
    ASSERT(is_keyword(grammar::KW_USING), "Expected 'using'");
    token* tok = next(); // using
    
    ast* ret = ast::make_binary({grammar::KW_USING}, tok, tt.TYPELESS);
    
    ret->binary.left = expression();
    
    if (is_symbol(grammar::KW_AS)) {
        next(); // as
        
        if (is(token_type::IDENTIFIER)) {
            ret->binary.right = identifier();
        } else {
            // TODO err
            ret->binary.right = ast::make_none({}, c, tt.TYPELESS);
            if (!is_symbol(grammar::SEMICOLON)) {
                next(); // ???
            }
            mod.errors.push_back({ret, "Invalid identifier"});
        }
    } else {
        ret->binary.right = ast::make_none({}, c, tt.TYPELESS);
    }
    
    // TODO err
    if (require_symbol(grammar::SEMICOLON)) next(); // ;
    
    return ret;
}


ast* pass::namespacestmt() {
    ASSERT(is_keyword(grammar::KW_NAMESPACE), "Expected 'namespace'");
    token* tok = next(); // namespace
    
    ast* ret = ast::make_binary({grammar::KW_NAMESPACE}, tok, tt.TYPELESS);
    
    if (is(token_type::IDENTIFIER)) {
        ret->binary.left = compound_identifier_simple();
    } else {
        // TODO err
        ret->binary.right = ast::make_none({}, c, tt.TYPELESS);
        if (!is_symbol(grammar::SEMICOLON)) {
            next(); // ???
        }
        mod.errors.push_back({ret, "Invalid identifier"});
    }
    
    if (require_symbol(grammar::OBRACE)) {
        ret->binary.right = namespacescope();
    }
    
    return ret;
}

ast* pass::namespacescope() {
    ASSERT(is_symbol(grammar::OBRACE), "Expected 'brace'");
    
    ast* ret = ast::make_block({}, c, tt.TYPELESS);
    
    next(); // {
    
    while (!is_symbol(grammar::CBRACE) && !is(token_type::END_OF_FILE)) {
        while (is(token_type::COMPILER)) {
            note(); // Ignore for now
        }
        
        require(token_type::KEYWORD); // ???
        
        switch (c->value) {
            case grammar::KW_USING:
                ret->block.elems.push_back(usingstmt());
                break;
            case grammar::KW_NAMESPACE:
                ret->block.elems.push_back(namespacestmt());
                break;
            case grammar::KW_DEF: [[fallthrough]];
            case grammar::KW_VAR: [[fallthrough]];
            case grammar::KW_REF: [[fallthrough]];
            case grammar::KW_LET:
                ret->block.elems.push_back(declstmt());
                break;
            default:
                // TODO err
                ast* ret = make_error_ast(c);
                if (!is_symbol(grammar::CBRACE)) {
                    next(); // ???
                }
                ret->block.elems.push_back(ret);
                break;
        }
    }
    
    // TODO err
    if (require_symbol(grammar::CBRACE)) next(); // }
    
    return ret;
}


// ast* pass::declarator() {
//     return nullptr;
// }
// 
ast* pass::_type() {
    token* tok = c;
    ast* t = expression();
    ast* ret = ast::make_unary({grammar::COLON, t}, tok, tt.TYPE);
    ret->compiled = t->compiled; // If it's trivial, this is also just that
    return ret;
}

ast* pass::inferrable_type(bool* is_infer) {
    token* tok = c;
    
    if (is_keyword(grammar::KW_INFER)) {
        next(); // infer
        ast* ret = ast::make_unary({grammar::COLON, ast::make_nntype({tt.INFER}, tok, tt.INFER)}, c, tt.TYPE);
        ret->compiled = ret->unary.node;
        if (is_infer) {
            *is_infer = true;
        }
        return ret;
    } else {
        ast* t = expression();
        ast* ret = ast::make_unary({grammar::COLON, t}, tok, tt.TYPE);
        ret->compiled = t->compiled; // Same as above
        if (is_infer) {
            *is_infer = false;
        }
        return ret;
    }
}


// ast* pass::paramtype() {
//     return nullptr;
// }
// 
// ast* pass::rettype() {
//     return nullptr;
// }


ast* pass::declstmt() {
    if (is_keyword(grammar::KW_DEF)) {
        return defstmt();
    } else if (is_keyword(grammar::KW_VAR) || is_keyword(grammar::KW_LET) || is_keyword(grammar::KW_REF)) {
        ast* ret = vardecl();
        
        if (require_symbol(grammar::SEMICOLON)) next(); // ;
        
        return ret;
    } else {
        ASSERT(false, "Expected 'def', 'var', 'let' or 'ref'");
        return nullptr; // aaaa
    }
}

ast* pass::defstmt() {
    ASSERT(is_keyword(grammar::KW_DEF), "Expected 'def'");
    
    token* tok = next(); // def
    
    require(token_type::KEYWORD);
    
    ast* ret{nullptr};
    
    switch (c->value) {
        case grammar::KW_FUN:
            ret = funcdef();
            break;
        case grammar::KW_STRUCT:
            ret = structtypelitdef();
            break;
        case grammar::KW_UNION:
            ret = uniontypelitdef();
            break;
        case grammar::KW_ENUM:
            ret = enumtypelitdef();
            break;
        case grammar::KW_TUPLE:
            ret = tupletypelitdef();
            break;
        default:
            // TODO err
            ast* ret = make_error_ast(c);
            next(); // ???
            return ret;
    }
    
    return ast::make_unary({grammar::KW_DEF, ret}, tok, tt.TYPELESS);
}

ast* pass::maybe_identifier() {
    if (is(token_type::IDENTIFIER)) {
        return identifier();
    } else if (is_keyword(grammar::KW_PLACEHOLDER)) {
        token* plac = next(); // _
        return ast::make_zero({grammar::KW_PLACEHOLDER}, plac, tt.TYPELESS);
    } else {
        return ast::make_zero({grammar::KW_PLACEHOLDER}, c, tt.TYPELESS);
    }
}

ast* pass::vardecl(bool special) {
    if (is_keyword(grammar::KW_VAR) || is_keyword(grammar::KW_LET) || is_keyword(grammar::KW_REF)) {
        token* tok = next();
        grammar::symbol sym = (grammar::symbol) tok->value; // var let ref
        
        return ast::make_unary({sym, simplevardecl(special)}, tok, tt.TYPELESS);
        
    } else {
        ASSERT(false, "Expected 'var', 'let' or 'ref'");
        return nullptr; // nu >:(
    }
}

ast* pass::simplevardecl(bool special) {
    ast* ret = ast::make_binary({grammar::KW_DEF}, c, tt.TYPELESS);
    ast* left = ret->binary.left = ast::make_block({}, c, tt.TYPELESS);
    bool any_infer{false}, named{false}, typed{false};
    do {
        left->block.elems.push_back(
            ast::make_binary({grammar::KW_DEF, ast::make_block({}, c, tt.TYPELESS)}, c, tt.TYPELESS)
        );
        ast* group = left->block.elems.tail;
        ast* block = group->binary.left;
        named = false;
        typed = false;
        do {
            if (is(token_type::IDENTIFIER)) {
                block->block.elems.push_back(identifier());
                named = true;
            } else if (is_keyword(grammar::KW_PLACEHOLDER)) {
                token* tok = next(); // _
                block->block.elems.push_back(ast::make_zero({grammar::KW_PLACEHOLDER}, tok, tt.TYPELESS));
                named = true;
            } else if (is_symbol(grammar::COLON)) {
                bool is_infer{false};
                token* tok = next(); // :
                group->binary.right = inferrable_type(&is_infer);
                group->binary.right->unary.sym = grammar::COLON;
                group->binary.right->tok = tok;
                typed = true;
                any_infer = is_infer;
                break;
            } else if (is_symbol(grammar::ASSIGN)) {
                token* tok = c;
                group->binary.right = ast::make_unary({grammar::COLON, 
                    ast::make_nntype({tt.INFER}, tok, tt.TYPE)}, tok, tt.TYPELESS);
                if (!named && !typed) {
                    // TODO error
                    ast* err = make_error_ast(c);
                    block->block.at_end.push_back(err);
                    mod.errors.push_back({err, "Cannot assign to unnamed untyped variable(s)"});
                }
                typed = true;
                any_infer = true;
                goto out_while;
            } else {
                // TODO Semicolon error
                require_symbol(grammar::COMMA);
                block->block.elems.push_back(ast::make_zero({grammar::KW_PLACEHOLDER}, c, tt.TYPELESS));
            }
        } while (is_symbol(grammar::COMMA) && next()); // ,
        
        if (is_symbol(grammar::COLON)) {
            bool is_infer{false};
            token* colon = next(); // :
            ast* typ = inferrable_type(&is_infer);
            if (!typed) {
                group->binary.right = typ;
                typed = true;
                any_infer = is_infer;
            } else {
                ast* err = make_error_ast(colon);
                block->block.at_end.push_back(err);
                mod.errors.push_back({err, "Already gave a type"});
            }
        } else if (!typed) {
            group->binary.right = ast::make_unary({grammar::COLON, 
                ast::make_nntype({tt.INFER}, c, tt.TYPE)}, c, tt.TYPELESS);
            typed = true;
            any_infer = true;
            if (!named) {
                ast* err = make_error_ast(c);
                block->block.at_end.push_back(err);
                mod.errors.push_back({err, "Unnamed untyped variable"});
            }
        }
        // We're here after a type, but there could be more shit
    } while (is_symbol(grammar::COMMA) && next()); // ,
    
    out_while:
    
    // TODO Extra colon error
    
    if (is_symbol(grammar::ASSIGN)) {
        ret->binary.right = assignment();
    } else {
        ret->binary.right = ast::make_none({}, c, tt.TYPELESS);
        if (any_infer && !special) {
            ast* err = make_error_ast(c);
            left->block.at_end.push_back(err);
            mod.errors.push_back({err, "Must assign to inferred types"});
        }
    }
    
    return ret;
}


ast* pass::funclit_or_type() {
    ASSERT(is_keyword(grammar::KW_FUN), "Expected 'fun'");
    token* tok = next(); // fun
    
    if (is_symbol(grammar::OBRACK)) {
        ast* ret = ast::make_compound({}, tok, tt.NONE_FUNCTION);
        ret->compound.elems.push_back(capture_group());
        
        ret->compound.elems.push_back(maybe_identifier());
        
        if (require_symbol(grammar::OPAREN)) {
            ret->compound.elems.push_back(functypesig());
        } else {
            // TODO err
            ret->compound.elems.push_back(make_error_ast(c));
        }
        
        if (require_symbol(grammar::OBRACE)) {
            ret->compound.elems.push_back(scope());
        } else {
            // TODO err
            ret->compound.elems.push_back(make_error_ast(c));
            next(); // ???
        }
        
        return ret;
    } else {
        ast* ret = ast::make_binary({grammar::KW_FUN}, tok, tt.TYPE);
        ret->binary.left = maybe_identifier();
        
        if (require_symbol(grammar::OPAREN)) {
            ret->binary.right = functypesig();
        } else {
            // TODO err
            ret->binary.right = make_error_ast(c);
            next(); // ???
        }
        
        return ret;
    }
}

ast* pass::funclit() {
    ASSERT(is_keyword(grammar::KW_FUN), "Expected 'fun'");
    
    token* tok = next(); // fun
    
    ast* ret = ast::make_compound({}, tok, tt.NONE_FUNCTION);
    
    if (require_symbol(grammar::OBRACK)) {
        ret->compound.elems.push_back(capture_group());
    } else {
        // TODO err
        ret->compound.elems.push_back(make_error_ast(c));
    }
    
    ret->compound.elems.push_back(maybe_identifier());
    
    if (require_symbol(grammar::OPAREN)) {
        ret->compound.elems.push_back(functypesig());
    } else {
        // TODO err
        ret->compound.elems.push_back(make_error_ast(c));
    }
    
    if (require_symbol(grammar::OBRACE)) {
        ret->compound.elems.push_back(scope());
    } else {
        // TODO err
        ret->compound.elems.push_back(make_error_ast(c));
        next(); // ???
    }
    
    return ret;
}

ast* pass::funcdef() {
    ASSERT(is_keyword(grammar::KW_FUN), "Expected 'fun'");
    
    token* tok = next(); // fun
    
    ast* ret = ast::make_compound({}, tok, tt.NONE_FUNCTION);
    
    if (is_symbol(grammar::OBRACK)) {
        ret->compound.elems.push_back(capture_group());
    } else {
        ret->compound.elems.push_back(ast::make_zero({grammar::SP_CAPTURE}, c, tt.TYPELESS));
    }
    
    if (require(token_type::IDENTIFIER)) {
        ret->compound.elems.push_back(identifier());
    } else {
        // TODO err
        ret->compound.elems.push_back(make_error_ast(c));
    }
    
    if (require_symbol(grammar::OPAREN)) {
        ret->compound.elems.push_back(functypesig());
    } else {
        // TODO err
        ret->compound.elems.push_back(make_error_ast(c));
    }
    
    // TODO Declarations without definitions
    if (require_symbol(grammar::OBRACE)) {
        ret->compound.elems.push_back(scope());
    } else {
        // TODO err
        ret->compound.elems.push_back(make_error_ast(c));
        next(); // ???
    }
    
    return ret;
}

ast* pass::capture_group() {
    ASSERT(is_symbol(grammar::OBRACK), "Expected [");
    
    ast* ret = ast::make_compound({}, c, tt.NONE_TUPLE);
    next(); // [
    
    if (!is_symbol(grammar::CBRACK)) {
        do {            
            if (is_symbol(grammar::POINTER)) {
                token* ptr = next(); // *
                
                if (require(token_type::IDENTIFIER)) {
                    ret->compound.elems.push_back(ast::make_unary({grammar::POINTER, identifier()}, ptr, tt.NONE));
                } else {
                    // TODO err
                    ret->compound.elems.push_back(make_error_ast(c));
                    next(); // ???
                }
                
            } else if (require(token_type::IDENTIFIER)) {
                
                ast* bin = ast::make_binary({grammar::ASSIGN, identifier()}, c, tt.TYPELESS);
                
                if (is_symbol(grammar::ASSIGN)) {
                    bin->tok = c;
                    bin->binary.right = assignment();
                } else {
                    bin->binary.right = ast::make_none({}, c, tt.TYPELESS);
                }
                
                ret->compound.elems.push_back(bin);
                
            } else {
                // TODO err
                ret->compound.elems.push_back(make_error_ast(c));
                next(); // ???
            }
        } while (is_symbol(grammar::COMMA) && next());
    }
    
    // TODO err
    if (require_symbol(grammar::CBRACK)) next(); // ]
    
    return ret;
}

ast* pass::functypesig() {
    ASSERT(is_symbol(grammar::OPAREN), "Expected (");
    token* tok = next(); // ( 
    ast* ret = ast::make_binary({grammar::COLON}, tok, tt.NONE_FUNCTION);
    ast* params = ret->binary.left = ast::make_compound({}, tok, tt.TYPELESS);
    
    if (!is_symbol(grammar::CPAREN) && !is_symbol(grammar::RARROW) && 
        !is_symbol(grammar::SRARROW) && !is(token_type::END_OF_FILE)) {
        do {
            params->compound.elems.push_back(funcparam());
        } while (is_symbol(grammar::COMMA) && next());
    }
    
    bool let{false};
    // TODO err
    ast* returns = ret->binary.right = ast::make_unary({grammar::RARROW}, tok, tt.TYPELESS);
    
    if (is_symbol(grammar::SRARROW) || is_symbol(grammar::RARROW)) {
        tok = next(); // -> =>
        let = tok->value == grammar::SRARROW;
        returns->unary.sym = (grammar::symbol) tok->value;
        
        if (is_keyword(grammar::KW_INFER)) {
            tok = next(); // infer
            returns->unary.node = ast::make_nntype({tt.INFER}, tok, tt.TYPE);
        } else {
            ast* comp = returns->unary.node = ast::make_compound({}, tok, tt.TYPELESS);
            
            if (!is_symbol(grammar::CPAREN)) {
                comp->compound.elems.push_back(funcret(let));
                while (is_symbol(grammar::COMMA)) {
                    next(); // ,
                    comp->compound.elems.push_back(funcret(let));
                }
            }
        }
        
    } else {
        returns->unary.node = ast::make_nntype({tt.INFER}, tok, tt.TYPE);
    }
    
    // TODO err
    if (require_symbol(grammar::CPAREN)) next(); // )
    
    return ret;
}

ast* pass::funcparam() {
    ast* ret = ast::make_binary({}, c, tt.TYPELESS);
    bool declarator{false};
    
    if (is_keyword(grammar::KW_LET)) {
        token* tok = next(); // let
        ret->binary.sym = grammar::KW_LET;
        ret->tok = tok;
        declarator = true;
    } else if (is_keyword(grammar::KW_VAR)) {
        token* tok = next(); // var
        ret->binary.sym = grammar::KW_VAR;
        ret->tok = tok;
        declarator = true;
    } else if (is_keyword(grammar::KW_REF)) {
        token* tok = next(); // ref
        ret->binary.sym = grammar::KW_REF;
        ret->tok = tok;
        declarator = true;
    } else {
        ret->binary.sym = grammar::KW_VAR;
    }
    
    ast* left = ret->binary.left = ast::make_binary({grammar::ASSIGN}, c, tt.TYPELESS);
    ast* right = ret->binary.right = ast::make_binary({grammar::COLON}, c, tt.TYPELESS);
    
    if (declarator) {
        left->binary.left = maybe_identifier();
        if (is_symbol(grammar::DCOLON)) {
            right->binary.sym = grammar::DCOLON;
            next(); // ::
        } else if (require_symbol(grammar::COLON)) {
            // TODO err
            next(); // :
        }
    } else if (is_symbol(grammar::DCOLON) || is_symbol(grammar::COLON)) {
        left->binary.left = maybe_identifier();
        right->binary.sym = (grammar::symbol) next()->value; // ::        
    } else {
        ast* first = expression();
        
        if (is_symbol(grammar::DCOLON) || is_symbol(grammar::COLON)) {
            
            left->binary.left = first; // It's the name
            if (first->tt != ast_type::IDENTIFIER) {
                // TODO err
                mod.errors.push_back({first, "Paramter name must be an identifier"});
            } 
            
            right->binary.sym = (grammar::symbol) next()->value; // ::
        } else {
            left->binary.left = ast::make_zero({grammar::KW_PLACEHOLDER}, c, tt.TYPELESS);
            // It's the type
            right->binary.left = first;
            goto past_type;
        }
    }
    
    right->binary.left = _type();
    
    past_type:
    
    if (is_symbol(grammar::SPREAD)) {
        right->binary.right = ast::make_zero({grammar::SPREAD}, next(), tt.TYPELESS); // ...
    } else {
        right->binary.right = ast::make_none({}, c, tt.TYPELESS);
    }
    
    if (is_symbol(grammar::ASSIGN)) {
        next(); // =
        left->binary.right = expression();
    } else {
        left->binary.right = ast::make_none({}, c, tt.TYPELESS);
    }
    
    return ret;
}

ast* pass::funcret(bool let) {
    ast* ret = ast::make_binary({}, c, tt.TYPELESS);
    bool declarator{false};
    
    if (is_keyword(grammar::KW_LET)) {
        token* tok = next(); // let
        ret->binary.sym = grammar::KW_LET;
        ret->tok = tok;
        declarator = true;
    } else if (is_keyword(grammar::KW_VAR)) {
        token* tok = next(); // var
        ret->binary.sym = grammar::KW_VAR;
        ret->tok = tok;
        declarator = true;
    } else if (is_keyword(grammar::KW_REF)) {
        token* tok = next(); // ref
        ret->binary.sym = grammar::KW_REF;
        ret->tok = tok;
        declarator = true;
    } else {
        ret->binary.sym = let ? grammar::KW_LET : grammar::KW_VAR;
    }
    
    if (declarator) {
        ret->binary.left = maybe_identifier(); // Name and colon obligatory
    } else if (is_symbol(grammar::COLON)) {
        ret->binary.left = maybe_identifier(); // Name not there, this'll give us what we want
    } else {
        if (is_keyword(grammar::KW_INFER)) {
            ret->binary.left = ast::make_zero({grammar::KW_PLACEHOLDER}, c, tt.TYPELESS);
            ret->binary.right = inferrable_type();
            
            return ret;
        } else {
            ast* first = expression();
            
            if (is_symbol(grammar::COLON)) {
                ret->binary.left = first;
                
                if (first->tt != ast_type::IDENTIFIER) {
                    // TODO err
                    mod.errors.push_back({first, "Return name must be an identifier"});
                } 
                
            } else {
                ret->binary.left = ast::make_zero({grammar::KW_PLACEHOLDER}, c, tt.TYPELESS);
                ret->binary.right = first;
                
                return ret;
            }
        }
    }
    
    // token* tok = c;
    // TODO err
    if (require_symbol(grammar::COLON)) next(); // :
    
    ret->binary.right = inferrable_type();
    
    return ret;
}


ast* pass::structtypelitdef() {
    ASSERT(is_keyword(grammar::KW_STRUCT), "Expected 'struct'");
    
    token* tok = next(); // struct
    
    ast* ret = ast::make_binary({grammar::KW_STRUCT}, tok, tt.TYPE);
    // TODO err
    if (require(token_type::IDENTIFIER)) {
        ret->binary.left = identifier();
    } else {
        ret->binary.left = make_error_ast(c);
    }
    
    if (is_symbol(grammar::OBRACE)) {
        ret->binary.right = structscope();
    } else {
        ret->binary.right = make_error_ast(c);
    }
    
    return ret;
}

ast* pass::structscope() {
    ASSERT(is_symbol(grammar::OBRACE), "Expected {");
    token* tok = next(); // {
    
    ast* ret = ast::make_block({}, tok, tt.TYPELESS);
    
    while (!is_symbol(grammar::CBRACE) && !is(token_type::END_OF_FILE)) {
        while (is(token_type::COMPILER)) {
            note(); // Ignore for now
        }
        
        // TODO err
        if (require(token_type::KEYWORD)) {
            switch (c->value) {
                case grammar::KW_DEF: 
                    ret->block.elems.push_back(defstmt());
                    break;
                case grammar::KW_VAR: [[fallthrough]];
                case grammar::KW_LET: [[fallthrough]];
                case grammar::KW_REF: {
                    ret->block.elems.push_back(structvardecl());
                    
                    // TODO err
                    if (require_symbol(grammar::SEMICOLON)) next(); // ;
                    
                    break;
                }
                default: {
                    // TODO err
                    ast* err = make_error_ast(c);
                    mod.errors.push_back({err, ss::get() << "Unexpected keyword " << (grammar::symbol) c->value << ss::end()});
                    next(); // ???
                }
            }
        } else {
            next(); // ???
        }
    }
    
    // TODO err
    if (require_symbol(grammar::CBRACE)) next();
    
    return ret;
}

ast* pass::structvardecl() {
    ASSERT(is_keyword(grammar::KW_LET) || is_keyword(grammar::KW_VAR) || is_keyword(grammar::KW_REF), "Expected 'var', 'let' or 'ref'");
    token* tok = next(); // var let ref
    
    ast* vardecl = ast::make_binary({grammar::KW_DEF}, tok, tt.TYPELESS);
    ast* left = vardecl->binary.left = ast::make_block({}, tok, tt.TYPELESS);
    bool any_infer{false}, named{false}, typed{false};
    do {
        left->block.elems.push_back(
            ast::make_binary({grammar::KW_DEF, ast::make_block({}, c, tt.TYPELESS)}, c, tt.TYPELESS)
        );
        ast* group = left->block.elems.tail;
        ast* block = group->binary.left;
        named = false;
        typed = false;
        do {
            if (is(token_type::IDENTIFIER)) {
                block->block.elems.push_back(identifier());
                named = true;
            } else if (is_keyword(grammar::KW_PLACEHOLDER)) {
                token* tok = next(); // _
                block->block.elems.push_back(ast::make_zero({grammar::KW_PLACEHOLDER}, tok, tt.TYPELESS));
                named = true;
            } else if (is_symbol(grammar::COLON) || is_symbol(grammar::DCOLON)) {
                bool is_infer{false};
                token* tok = next(); // : ::
                group->binary.right = inferrable_type(&is_infer);
                group->binary.right->unary.sym = (grammar::symbol) tok->value;
                group->binary.right->tok = tok;
                typed = true;
                any_infer = is_infer; // TODO DCOLON + INFER error
                break;
            } else if (is_symbol(grammar::ASSIGN)) {
                token* tok = c;
                group->binary.right = ast::make_unary({grammar::COLON, 
                    ast::make_nntype({tt.INFER}, tok, tt.TYPE)}, tok, tt.TYPELESS);
                if (!named && !typed) {
                    // TODO error
                    ast* err = make_error_ast(c);
                    block->block.at_end.push_back(err);
                    mod.errors.push_back({err, "Cannot assign to unnamed untyped variable(s)"});
                }
                typed = true;
                any_infer = true;
                goto out_while;
            } else {
                // TODO Semicolon error
                require_symbol(grammar::COMMA);
                block->block.elems.push_back(ast::make_zero({grammar::KW_PLACEHOLDER}, c, tt.TYPELESS));
            }
        } while (is_symbol(grammar::COMMA) && next()); // ,
        
        if (is_symbol(grammar::COLON) || is_symbol(grammar::DCOLON)) {
            bool is_infer{false};
            token* tok = next(); // : ::
            ast* typ = inferrable_type(&is_infer);
            if (!typed) {
                group->binary.right = typ;
                group->binary.right->unary.sym = (grammar::symbol) tok->value;
                group->binary.right->tok = tok;
                typed = true;
                any_infer = is_infer;
            } else {
                ast* err = make_error_ast(tok);
                block->block.at_end.push_back(err);
                mod.errors.push_back({err, "Already gave a type"});
            }
        } else if (!typed) {
            group->binary.right = ast::make_unary({grammar::COLON, 
                ast::make_nntype({tt.INFER}, c, tt.TYPE)}, c, tt.TYPELESS);
            typed = true;
            any_infer = true;
            if (!named) {
                ast* err = make_error_ast(c);
                block->block.at_end.push_back(err);
                mod.errors.push_back({err, "Unnamed untyped variable"});
            }
        }
        
        // We're here after a type, but there could be more shit
    } while (is_symbol(grammar::COMMA) && next()); // ,
    
    out_while:
    
    if (is_symbol(grammar::ASSIGN)) {
        vardecl->binary.right = assignment();
    } else {
        vardecl->binary.right = ast::make_none({}, c, tt.TYPELESS);
        if (any_infer) {
            ast* err = make_error_ast(c);
            left->block.at_end.push_back(err);
            mod.errors.push_back({err, "Must assign to inferred types"});
        }
    }
    
    return vardecl;
}


ast* pass::uniontypelitdef() {
    ASSERT(is_keyword(grammar::KW_UNION), "Expected 'union'");
    
    token* tok = next(); // union
    
    ast* ret = ast::make_binary({grammar::KW_UNION}, tok, tt.TYPE);
    // TODO err
    if (require(token_type::IDENTIFIER)) {
        ret->binary.left = identifier();
    } else {
        ret->binary.left = make_error_ast(c);
    }
    
    // TODO err
    if (is_symbol(grammar::OBRACE)) {
        ret->binary.right = unionscope();
    } else {
        ret->binary.right = make_error_ast(c);
    }
    
    return ret;
}

ast* pass::unionscope() {
    ASSERT(is_symbol(grammar::OBRACE), "Expected {");
    token* tok = next(); // {
    
    ast* ret = ast::make_block({}, tok, tt.TYPELESS);
    
    while (!is_symbol(grammar::CBRACE) && !is(token_type::END_OF_FILE)) {
        while (is(token_type::COMPILER)) {
            note(); // Ignore for now
        }
        
        require(token_type::KEYWORD);
        switch (c->value) {
            case grammar::KW_DEF: 
                ret->block.elems.push_back(defstmt());
                break;
            case grammar::KW_VAR: [[fallthrough]];
            case grammar::KW_LET: [[fallthrough]];
            case grammar::KW_REF: {
                ret->block.elems.push_back(vardecl());
                
                // TODO err
                if (require_symbol(grammar::SEMICOLON)) next(); // ;
                
                break;
            }
            default: {
                // TODO err
                ast* err = make_error_ast(c);
                mod.errors.push_back({err, ss::get() << "Unexpected keyword " << (grammar::symbol) c->value << ss::end()});
                next(); // ???
            }
        }
    }
    
    // TODO err
    if (require_symbol(grammar::CBRACE)) next();
    
    return ret;
}

ast* pass::enumtypelitdef() {
    ASSERT(is_keyword(grammar::KW_ENUM), "Expected 'enum'");
    
    token* tok = next(); // enum
    
    ast* ret = ast::make_binary({grammar::KW_ENUM}, tok, tt.TYPE);
    // TODO err
    if (require(token_type::IDENTIFIER)) {
        ret->binary.left = identifier();
    } else {
        ret->binary.left = make_error_ast(c);
    }
    
    // TODO err
    if (is_symbol(grammar::OBRACE)) {
        ret->binary.right = enumscope();
    } else {
        ret->binary.right = make_error_ast(c);
    }
    
    return ret;
}

ast* pass::enumscope() {
    ASSERT(is_symbol(grammar::OBRACE), "Expected {");
    token* tok = next(); // {
    
    ast* ret = ast::make_block({}, tok, tt.TYPELESS);
    
    if (!is_symbol(grammar::CBRACE) && !is(token_type::END_OF_FILE)) {
        do {
            while (is(token_type::COMPILER)) {
                note(); // Ignore for now
            }
            
            ast* elem = ast::make_binary({grammar::SP_NAMED}, c, tt.NONE);
            
            if (require(token_type::IDENTIFIER)) {
                elem->binary.left = identifier();
            } else {
                elem->binary.left = make_error_ast(c);
            }
            
            if (is_symbol(grammar::ASSIGN)) {
                next(); // 
                elem->binary.right = expression();
            } else {
                elem->binary.right = ast::make_none({}, c, tt.TYPELESS);
            }
            
            ret->block.elems.push_back(elem);
        } while (is_symbol(grammar::COMMA) && next());
    }
    
    // TODO err
    if (require_symbol(grammar::CBRACE)) next();
    
    return ret;
}


ast* pass::tupletypelit() {
    ASSERT(is_keyword(grammar::KW_TUPLE), "Expected 'tuple'");
    
    token* tok = next(); // tuple
    
    ast* ret = ast::make_binary({grammar::KW_TUPLE}, tok, tt.TYPE);
    ret->binary.left = maybe_identifier();
    
    // TODO err
    if (is_symbol(grammar::OPAREN)) {
        ret->binary.right = tupletypes();
    } else {
        ret->binary.right = make_error_ast(c);
    }
    
    return ret;
}

ast* pass::tupletypelitdef() {
    ASSERT(is_keyword(grammar::KW_TUPLE), "Expected 'tuple'");
    
    token* tok = next(); // tuple
    
    ast* ret = ast::make_binary({grammar::KW_TUPLE}, tok, tt.TYPE);
    // TODO err
    if (require(token_type::IDENTIFIER)) {
        ret->binary.left = identifier();
    } else {
        ret->binary.left = make_error_ast(c);
    }
    
    // TODO err
    if (is_symbol(grammar::OPAREN)) {
        ret->binary.right = tupletypes();
    } else {
        ret->binary.right = make_error_ast(c);
    }
    
    return ret;
}

ast* pass::tupletypes() {
    ASSERT(is_symbol(grammar::OPAREN), "Expected (");
    token* tok = next(); // (
    
    ast* ret = ast::make_block({}, tok, tt.TYPELESS);
    
    if (!is_symbol(grammar::OPAREN) && !is(token_type::END_OF_FILE)) {
        do {            
            ret->block.elems.push_back(_type());
        } while (is_symbol(grammar::COMMA) && next());
    }
    
    // TODO err
    if (require_symbol(grammar::CPAREN)) next();
    
    return ret;
}


// ast* pass::typelit() {
//     return nullptr;
// }

ast* pass::typelit_nofunc() {
    
    require(token_type::KEYWORD);
    
    type* t{nullptr};
    
    switch (c->value) {
        case grammar::KW_U0: t = tt.U0; break;
        case grammar::KW_U1: t = tt.U1; break;
        case grammar::KW_U8: t = tt.U8; break;
        case grammar::KW_U16: t = tt.U16; break;
        case grammar::KW_U32: t = tt.U32; break;
        case grammar::KW_U64: t = tt.U64; break;
        case grammar::KW_S8: t = tt.S8; break;
        case grammar::KW_S16: t = tt.S16; break;
        case grammar::KW_S32: t = tt.S32; break;
        case grammar::KW_S64: t = tt.S64; break;
        case grammar::KW_F32: t = tt.F32; break;
        case grammar::KW_F64: t = tt.F64; break;
        case grammar::KW_C8: t = tt.C8; break;
        case grammar::KW_C16: t = tt.C16; break;
        case grammar::KW_C32: t = tt.C32; break;
        case grammar::KW_E64: t = tt.E64; break;
        case grammar::KW_TYPE: t = tt.TYPE; break;
        case grammar::KW_ANY: t = tt.ANY; break;
        case grammar::KW_TUPLE: return tupletypelit();
        default:
            ast* ret = make_error_ast(next());
            mod.errors.push_back({ret, ss::get() << "Expected type literal but got" << 
                                       (grammar::symbol) c->value << ss::end()});
            return ret;
    }
    
    token* tok = next(); // u0 u1 u8 u16 u32 u64 s8 s16 s32 s64 f32 f64 c8 c16 c32 e64 type any
    
    return ast::make_nntype({t}, tok, tt.TYPE);
}


ast* pass::assignment() {
    ASSERT(is_assign(), "Expected =");
    token* tok = next(); // =
    
    ast* ret = ast::make_block({}, tok, tt.TYPELESS);
    
    ret->block.elems.push_back(expression());
    
    while (is_symbol(grammar::COMMA)) {
        next(); // ,
        ret->block.elems.push_back(expression());
    }
    
    return ret;
}

ast* pass::assstmt() {   
    ast* ret = ast::make_binary({grammar::ASSIGN}, c, tt.TYPELESS);
    ast* left = ret->binary.left = ast::make_block({}, c, tt.TYPELESS);
    ast* right = ret->binary.right = ast::make_block({}, c, tt.TYPELESS);
    
    left->block.elems.push_back(expression());
    
    while (is_keyword(grammar::COMMA)) {
        next(); // ,
        left->block.elems.push_back(expression());
    }
    
    // TODO err
    if (require_assign()) {
        token* tok = next(); // =
        ret->binary.sym = (grammar::symbol) tok->value;
    }
    
    right->block.elems.push_back(expression());
    
    while (is_keyword(grammar::COMMA)) {
        next(); // ,
        right->block.elems.push_back(expression());
    }
    
    // TODO err
    if (require_symbol(grammar::SEMICOLON)) next(); // ;
    
    return ret;
}


ast* pass::deletestmt() {
    ASSERT(is_keyword(grammar::KW_DELETE), "Expected 'delete'");
    
    token* tok = next(); // delete
    
    ast* ret = ast::make_unary({grammar::KW_DELETE}, tok, tt.TYPELESS);
    ast* exprs = ret->unary.node = ast::make_block({}, tok, tt.TYPELESS);
    
    exprs->block.elems.push_back(expression());
    
    while (is_symbol(grammar::COMMA)) {
        next(); // ,
        exprs->block.elems.push_back(expression());
    }
    
    // TODO err
    if (require_symbol(grammar::SEMICOLON)) next(); // ;
    
    return ret;
}


ast* pass::expressionstmt() {
    ast* ret = expression();
    
    // TODO err
    if (require_symbol(grammar::SEMICOLON)) next(); // ;
    
    return ret;
}

ast* pass::assorexpr(bool stmt) {
    token* tok = c;
    ast* first = expression();
    
    if (is_symbol(grammar::SEMICOLON)) {
        if (stmt) next(); // ;
        return first;
    } else if (is_symbol(grammar::COMMA) || is_assign()) {
        ast* ret = ast::make_binary({grammar::ASSIGN}, tok, tt.TYPELESS);
        ast* left = ret->binary.left = ast::make_block({}, tok, tt.TYPELESS);
        ast* right = ret->binary.right = ast::make_block({}, tok, tt.TYPELESS);
        
        left->block.elems.push_back(first);
        
        while (is_symbol(grammar::COMMA)) {
            next(); // ,
            left->block.elems.push_back(expression());
        }
        
        // TODO err
        if (require_assign()) {
            token* tok = next(); // =
            ret->binary.sym = (grammar::symbol) tok->value;
        }
        
        right->block.elems.push_back(expression());
        
        while (is_keyword(grammar::COMMA)) {
            next(); // ,
            right->block.elems.push_back(expression());
        }
        
        // TODO err
        if (stmt && require_symbol(grammar::SEMICOLON)) next(); // ;
        
        return ret;
    } else {
        if (stmt) {
            mod.errors.push_back({first, ss::get() << "Invalid statement" << ss::end()});
        }
        return first;
    }
}

// Finds leftmost expression node in (right) child
// This is the node that must be adopted when 
// swapping tree nodes due to precedence
ast* find_leftmost(ast* from, s16 prec) {
    ast* n, * p{nullptr};
    switch (from->tt) {
        case ast_type::UNARY: 
            n = from->unary.node;
            break;
        case ast_type::BINARY:
            n = from->binary.right;
            break;
        default:
            ASSERT(false, "find_leftmost() called on non-ary");
            return from; // Never reached
    }
    
    if (n->precedence == -1 || n->precedence >= prec) {
        return p;
    }
    
    do {
        n->inhprecedence = prec;
        p = n;
        
        switch (n->tt) {
            case ast_type::UNARY: 
                n = n->unary.node;
                break;
            case ast_type::BINARY:
                n = n->binary.left;
                break;
            default:
                return p;
        }
        
        if (n->precedence == -1 || n->precedence >= prec) {
            return p;
        }
        
    } while (true);
}

// Precedence sorts tree starting at un
// Function return is new root of the tree, now sorted by precedence
// Subtree of un must already be sorted
ast* reorder_unary(ast* un) {
    ASSERT(un->tt == ast_type::UNARY, "ast passed to reorder_unary() wasn't unary");
    
    ast* ret = un;
    
    if (ret->precedence == -1) {
        return ret; // Nothing to do
    }
    
    switch (ret->tt) {
        case ast_type::UNARY: [[fallthrough]];
        case ast_type::BINARY: {
            ast* leftmost = find_leftmost(ret, ret->precedence);
            if (!leftmost) {
                return un;
            }
            switch (leftmost->tt) {
                case ast_type::UNARY: {
                    ast* leftmost_child = leftmost->unary.node;
                    leftmost->unary.node = ret;
                    ret->unary.node = leftmost_child;
                    ret = leftmost;
                    break;
                }
                case ast_type::BINARY: {
                    ast* leftmost_child = leftmost->binary.left;
                    leftmost->binary.left = ret;
                    ret->unary.node = leftmost_child;
                    ret = leftmost;
                    break;
                }
                default:
                    ASSERT(false, "find_leftmost() returned non-ary");
                    return un;
            }
            break;
        }
        default:
            break; // Nothing
    }
    
    return ret;
}

// Precedence sorts tree starting at bin
// Function return is new root of the tree, now sorted by precedence
// Right child tree of bin must already be sorted
ast* reorder_binary(ast* bin) {
    ASSERT(bin->tt == ast_type::BINARY, "ast passed to reorder_binary() wasn't binary");
    
    ast* ret = bin;
    
    if (ret->precedence == -1) {
        return ret; // Nothing to do
    }
    
    switch (ret->tt) {
        case ast_type::UNARY: [[fallthrough]];
        case ast_type::BINARY: {
            ast* leftmost = find_leftmost(ret, ret->precedence);
            if (!leftmost) {
                return bin; // Already at lowest level
            }
            switch (leftmost->tt) {
                case ast_type::UNARY: {
                    ast* leftmost_child = leftmost->unary.node;
                    leftmost->unary.node = ret;
                    ast* top = ret->binary.right;
                    ret->binary.right = leftmost_child;
                    ret = top;
                    ret = leftmost;
                    break;
                }
                case ast_type::BINARY: {
                    ast* leftmost_child = leftmost->binary.left;
                    leftmost->binary.left = ret;
                    ast* top = ret->binary.right;
                    ret->binary.right = leftmost_child;
                    ret = top;
                    break;
                }
                default:
                    ASSERT(false, "find_leftmost() returned non-ary");
                    return bin;
            }
            break;
        }
        default:
            break; // Nothing
    }
    return ret;
}

ast* pass::expression() {
    return ternaryexpr();
}

ast* pass::ternaryexpr() {
    ast* nexpr = newexpr();
    if (is_symbol(grammar::DQUESTION)) {
        token* t = next(); // ??
        ast* ternary = ast::make_binary({grammar::DQUESTION, nexpr, 
            ast::make_binary({grammar::CHOICE}, t, tt.NONE)}, t, tt.NONE);
        ternary->binary.left = expression();
        ternary->precedence = 0x0; // Lowest
        ternary->inhprecedence = nexpr->inhprecedence > 0 ? nexpr->inhprecedence : 0x0;
        // TODO err
        ternary->binary.right->tok = c;
        if (require_symbol(grammar::DIAMOND)) next(); // <>
        ternary->binary.right = expression();
        return ternary;
    } else {
        return nexpr;
    }
    
}

ast* pass::newexpr() {
    if (is_keyword(grammar::KW_NEW)) {
        token* tok = next(); // new
        
        ast* ret = ast::make_binary({grammar::KW_NEW}, tok, tt.NONE);
        ast* right = ret->binary.right = ast::make_binary({grammar::MUL}, tok, tt.TYPELESS);
        
        if (is_symbol(grammar::OPAREN)) {
            next(); // (
            right->binary.right = expression();
            
            // TODO err
            if (require_symbol(grammar::CPAREN)) next(); // )
        } else {
            right->binary.right = ast::make_value({1}, tok, tt.U64);
        }
        
        ast* first = expression();
        
        if (is_symbol(grammar::COLON)) {
            next(); // :
            ret->binary.left = first;
            right->binary.left = expression();
        } else {
            right->binary.left = first;
            ret->binary.left = ast::make_none({}, tok, tt.TYPELESS);
        }
        
        return ret;
    } else {
        return prefixexpr();
    }
}

ast* pass::prefixexpr() {
    if ((is(token_type::SYMBOL) || is(token_type::KEYWORD)) && pre_ops.count((grammar::symbol) c->value)) {
        if (!is_symbol(grammar::OBRACK)) {
            // For now
            token* tok = next(); // sym
            ast* ret = ast::make_unary({(grammar::symbol) tok->value, expression()}, tok, tt.NONE);
        
            if ((grammar::symbol) tok->value == grammar::SPREAD) {
                ret->precedence = 0x3E;
            } else {
                ret->precedence = 0x3C;
            }
            
            ret->inhprecedence = ret->unary.node->inhprecedence > ret->precedence ? ret->unary.node->inhprecedence : ret->inhprecedence;
            
            return reorder_unary(ret);
        } else {
            token* tok = next(); // [
            ast* ret = ast::make_binary({grammar::CBRACK}, tok, tt.NONE); // !!!
            if (!is_symbol(grammar::CBRACK)) {
                ret->binary.right = expression();
            } else {
                ret->binary.right = ast::make_none({}, tok, tt.TYPELESS);
            }
            
            // TODO err
            if (require_symbol(grammar::CBRACK)) next(); // ]
            ret->binary.left = expression();
            ret->precedence = 0x3C;
            ret->inhprecedence = ret->binary.left->inhprecedence > 0x3C ? ret->binary.left->inhprecedence : 0x3C;
            return reorder_binary(ret);
        }
    } else {
        return postfixexpr();
    }
}

ast* pass::postfixexpr() {
    ast* infix = infixexpr();
    
    while ((is(token_type::SYMBOL) || is(token_type::KEYWORD)) && post_ops.count((grammar::symbol) c->value)) {
        token* tok = next(); // sym
        s16 prev_prec = infix->precedence;
        infix = ast::make_unary({(grammar::symbol) tok->value, infix}, tok, tt.NONE);
        infix->unary.post = true;
        infix->precedence = 0x3F;
        infix->inhprecedence = prev_prec > 0x3F ? prev_prec : 0x3F;
        infix = reorder_unary(infix);
    }
    
    return infix;
}

ast* pass::infixexpr() {
    ast* dot = dotexpr(); 
    if ((is(token_type::SYMBOL) || is(token_type::KEYWORD)) && infix_ops.count((grammar::symbol) c->value)) {
        token* tok = next(); // sym
        grammar::symbol sym = (grammar::symbol) tok->value;
        ast* ret = ast::make_binary({sym, dot}, tok, tt.NONE);
        ast* right = ret->binary.right = expression();
        
        switch (sym) {
            case grammar::KW_AS:
                ret->precedence = 0x3F;
                break;
            case grammar::MUL:
                ret->precedence = 0x3B;
                break;
            case grammar::DIV:
                ret->precedence = 0x3B;
                break;
            case grammar::IDIV:
                ret->precedence = 0x3B;
                break;
            case grammar::MODULO:
                ret->precedence = 0x3B;
                break;
            case grammar::ADD:
                ret->precedence = 0x3A;
                break;
            case grammar::SUB:
                ret->precedence = 0x3A;
                break;
            case grammar::CONCAT:
                ret->precedence = 0x3A;
                break;
            case grammar::SHL:
                ret->precedence = 0x39;
                break;
            case grammar::SHR:
                ret->precedence = 0x39;
                break;
            case grammar::RTL:
                ret->precedence = 0x39;
                break;
            case grammar::RTR:
                ret->precedence = 0x39;
                break;
            case grammar::BITSET:
                ret->precedence = 0x38;
                break;
            case grammar::BITCLEAR:
                ret->precedence = 0x38;
                break;
            case grammar::BITTOGGLE:
                ret->precedence = 0x38;
                break;
            case grammar::BITCHECK:
                ret->precedence = 0x38;
                break;
            case grammar::AND:
                ret->precedence = 0x37;
                break;
            case grammar::OR:
                ret->precedence = 0x36;
                break;
            case grammar::XOR:
                ret->precedence = 0x35;
                break;
            case grammar::LT:
                ret->precedence = 0x34;
                break;
            case grammar::LE:
                ret->precedence = 0x34;
                break;
            case grammar::GT:
                ret->precedence = 0x34;
                break;
            case grammar::GE:
                ret->precedence = 0x34;
                break;
            case grammar::EQUALS:
                ret->precedence = 0x33;
                break;
            case grammar::NEQUALS:
                ret->precedence = 0x33;
                break;
            case grammar::LAND:
                ret->precedence = 0x32;
                break;
            case grammar::LOR:
                ret->precedence = 0x31;
                break;
            default:
                ASSERT(false, "Invalid infix operator");
                break;
        }
        
        ret->inhprecedence = right->inhprecedence > ret->precedence ? 
                             right->inhprecedence : 
                             ret->precedence;
        
        return reorder_binary(ret);
    } else {
        return dot;
    }
}

ast* pass::dotexpr() {
    ast* postcircumfix = postcircumfixexpr();
    
    if (is_symbol(grammar::PERIOD)) {
        token* tok = next(); // .
        ast* ret = ast::make_binary({grammar::PERIOD, postcircumfix}, tok, tt.NONE);
        
        if (is_symbol(grammar::MUL)) {
            token* tok = next(); // *
            ret->binary.right = ast::make_zero({grammar::MUL}, tok, tt.NONE);
        } else {
            ast* dot = dotexpr();
            
            if (dot->is_binary() && dot->binary.sym == grammar::PERIOD) {
                // Rotate left (Is this left?)
                
                ret->binary.right = dot->binary.left;
                dot->binary.left = ret;
                
            } else {
                ret->binary.right = dot;
            }
        }
        
        return ret; // Always precedence -1
    } else {
        return postcircumfix;
    }
}

ast* pass::postcircumfixexpr() {
    ast* literal = literalexpr(); // Everything below this always has precedence -1
    
    
    while (is(token_type::SYMBOL)) {
        bool require_reorder = false;
        s16 prec = -1;
        switch (c->value) {
            case grammar::OPAREN: {
                literal = ast::make_binary({grammar::OPAREN, literal, function_call()}, c, tt.NONE);
                prec = 0x3F;
                require_reorder = true;
                break;
            }
            case grammar::OBRACK: {
                literal = ast::make_binary({grammar::OBRACK, literal, access()}, c, tt.NONE);
                prec = 0x3F;
                require_reorder = true;
                break;
            }
            case grammar::OSELECT: {
                literal = ast::make_binary({grammar::OSELECT, literal, reorder()}, c, tt.NONE);
                prec = 0x3D;
                require_reorder = true;
                break;
            }
            default:
                return require_reorder ? reorder_binary(literal) : literal;
        }
        literal->precedence = prec;
        literal->inhprecedence = prec;
    } 
    
    return literal;
}

ast* pass::function_call() {
    ASSERT(is_symbol(grammar::OPAREN), "Expected (");
    
    token* tok = next(); // (
    ast* ret = ast::make_block({}, tok, tt.TYPELESS);
    
    if (!is_symbol(grammar::CPAREN)) {
        do {
            token* tok = c;
            ast* first = expression();
            if (is_symbol(grammar::ASSIGN)) {
                tok = next(); // =
                ret->block.elems.push_back(ast::make_binary({
                    grammar::SP_NAMED, 
                    first, 
                    expression()
                }, tok, tt.TYPELESS));
            } else {
                ret->block.elems.push_back(ast::make_binary({
                    grammar::SP_NAMED, 
                    ast::make_none({}, tok, tt.TYPELESS), 
                    first
                }, tok, tt.TYPELESS));
            }
        } while (is_symbol(grammar::COMMA) && next());
    }
    
    // TODO err
    if (require_symbol(grammar::CPAREN)) next(); // )
    
    return ret;
}

ast* pass::access() {
    ASSERT(is_symbol(grammar::OBRACK), "Expected [");
    next(); // [
    
    ast* ret = expression();
    
    // TODO err
    if (require_symbol(grammar::CBRACK)) next(); // ]
    
    return ret;
}

ast* pass::reorder() {
    ASSERT(is_symbol(grammar::OSELECT), "Expected ::[");
    token* tok = next(); // ::[
    ast* ret = ast::make_block({}, tok, tt.TYPELESS);
    
    do {
        ret->block.elems.push_back(expression());
    } while (is_symbol(grammar::COMMA) && next());
    
    // TODO err
    if (require_symbol(grammar::CBRACK)) next(); // ]
    
    return ret;
}


// ast* pass::literal() {
//     return nullptr;
// }

ast* pass::literalexpr() {
    ast* ret{nullptr};
    
    switch (c->tt) {
        case token_type::NUMBER: [[fallthrough]];
        case token_type::INTEGER: [[fallthrough]];
        case token_type::FLOATING:
            ret = number();
            break;
        case token_type::STRING:
            ret = string_lit();
            break;
        case token_type::CHARACTER:
            ret = char_lit();
            break;
        case token_type::SYMBOL:
            switch (c->value) {
                case grammar::LITERAL_ARRAY:
                    ret = array_lit();
                    break;
                case grammar::LITERAL_STRUCT:
                    ret = struct_lit();
                    break;
                case grammar::LITERAL_TUPLE:
                    ret = tuple_lit();
                    break;
                case grammar::NOTHING:
                    ret = ast::make_value({0}, next(), tt.NOTHING); // ---
                    break;
                default:
                    ret = identifierexpr();
                    break;
            }
            break;
        case token_type::KEYWORD:
            switch (c->value) {
                case grammar::KW_FALSE:
                    ret = ast::make_value({0}, next(), tt.U1); // false
                    break;
                case grammar::KW_TRUE:
                    ret = ast::make_value({1}, next(), tt.U1); // true
                    break;
                case grammar::KW_NULL:
                    ret = ast::make_value({0}, next(), tt.NULL_); // null
                    break;
                case grammar::KW_FUN:
                    ret = funclit_or_type();
                    break;
                case grammar::KW_THIS:
                    ret = ast::make_zero({grammar::KW_THIS}, next(), tt.NONE); // this
                    break;
                case grammar::KW_PLACEHOLDER:
                    ret = ast::make_zero({grammar::KW_PLACEHOLDER}, next(), tt.NONE); // _
                    break;
                default:
                    ret = typelit_nofunc();
                    break;
                
            }
            break;
        default:
            ret = identifierexpr();
            if (is_symbol(grammar::OSELECT)) {
                token* tok = c;
                ret = ast::make_binary({grammar::OSELECT, ret, select()}, tok, tt.NONE);
            }
            break;
    }
    
    return ret;
}

ast* pass::identifierexpr() {
    if (is_symbol(grammar::OPAREN)) {
        return parenexpr();
    } else if (is(token_type::IDENTIFIER)) {
        return identifier();
    } else {
        // Alright, it could be nothing else
        // TODO err
        ast* ret = make_error_ast(c);
        mod.errors.push_back({ret, "Invalid expression"});
        if (!is_symbol(grammar::SEMICOLON) && !is_symbol(grammar::OBRACE) && 
            !is_symbol(grammar::OPAREN) && !is_symbol(grammar::OBRACK)) {
                next(); // ???
        }
        return ret;
    }
}

ast* pass::parenexpr() {
    ASSERT(is_symbol(grammar::OPAREN), "Expected (");
    token* tok = next(); // (
    ast* exp = expression();
    ast* ret = ast::make_unary({grammar::CPAREN, exp}, tok, tt.NONE);
    ret->compiled = exp->compiled;
    // TODO err
    if (require_symbol(grammar::CPAREN)) next(); // )
    
    return ret;
}


ast* pass::select() {
    ASSERT(is_symbol(grammar::OSELECT), "Expected ::[");
    token* tok = next(); // ::[
    ast* ret = ast::make_block({}, tok, tt.TYPELESS);
    
    do {
        token* tok = c;
        ast* first = expression();
        if (is_symbol(grammar::ASSIGN)) {
            tok = next(); // =
            ret->block.elems.push_back(ast::make_binary({
                grammar::SP_NAMED, 
                first, 
                expression()
            }, tok, tt.TYPELESS));
        } else {
            ret->block.elems.push_back(ast::make_binary({
                grammar::SP_NAMED, 
                ast::make_none({}, tok, tt.TYPELESS), 
                first
            }, tok, tt.TYPELESS));
        }
    } while (is_symbol(grammar::COMMA) && next());
    
    // TODO err
    if (require_symbol(grammar::CBRACK)) next(); // ]
    
    return ret;
}

ast* pass::compound_identifier_simple() {
    ASSERT(is(token_type::IDENTIFIER), "Expected identifier");
    
    token* tok = c;
    ast* ret = ast::make_unary({grammar::PERIOD, ast::make_block({}, tok, tt.TYPELESS), true}, tok, tt.TYPELESS);
    ast* block = ret->unary.node;
    
    do {
        if (is_symbol(grammar::MUL)) {
            token* tok = next(); // *
            block->block.elems.push_back(ast::make_zero({grammar::MUL}, tok, tt.TYPELESS));
            break;
        } else {
            if (require(token_type::IDENTIFIER)) {
                block->block.elems.push_back(identifier());
            } else {
                // TODO err
                block->block.elems.push_back(make_error_ast(c));
                next(); // ???
            }
        }
    } while (is_symbol(grammar::PERIOD) && next());
    
    return ret;
}

ast* pass::make_error_ast(token* t) {
    errors.push_back(ast::make_none({}, t, nullptr));
    return errors.tail;
}
