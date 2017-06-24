#pragma once

#include <map>
#include <string>

#include "trie.h"

namespace Grammar {

enum class TokenType {
    KEYWORD,
    SYMBOL,
    IDENTIFIER,
    NUMBER,
    STRING,
    CHARACTER
};

enum class Keyword : char {
    /* Types */
    VOID, BYTE, CHAR, SHORT, INT,
    LONG, SIG, FLOAT, DOUBLE, BOOL,
    STRUCT, UNION, ENUM, STRING, FUN, LET,

    /* Type modifiers */
    CONST, VOLATILE,

    /* Control */
    IF, ELSE, FOR, WHILE,
    DO, SWITCH, RETURN,
    RAISE, BREAK, CONTINUE,
    LEAVE, GOTO, LABEL, DEFER,

    /* Other */
    TRUE, FALSE, _NULL, IMPORT, 
    USING, NAMESPACE, CASE, AS, 
    NEW, DELETE,

    /* Reserved */
    TRY, CATCH, AND, OR
};

enum class Symbol : char {
    POINTER = 0, MULTIPLY = 0, ADDRESS = 0,
    GREATER = 1, THAN_RIGHT = 1,
    LESS = 2, THAN_LEFT = 2,
    TERNARY_CHOICE = 3, COLON = 3,
    
    COMPILER,
    
    /*POINTER,*/ UNIQUE_POINTER,
    WEAK_POINTER, SHARED_POINTER,

    NOTHING,

    INCREMENT, DECREMENT,

    ADD, SUBSTRACT, /*MULTIPLY,*/ POWER,
    DIVIDE, MODULO,

    /*ADDRESS,*/ DEREFERENCE,

    NOT, AND, OR, XOR,

    LNOT, LAND, LOR, LXOR,

    ACCESS, CONCATENATE, SPREAD,

    TERNARY_CONDITION, /*TERNARY_CHOICE,*/

    BIT_SET, BIT_CLEAR, 
    BIT_CHECK, BIT_TOGGLE,

    SHIFT_LEFT, SHIFT_RIGHT, 
    ROTATE_LEFT, ROTATE_RIGHT,

    EQUALS, NOT_EQUALS,
    /*GREATER,*/ GREATER_OR_EQUALS,
    /*LESS,*/ LESS_OR_EQUALS,

    ASSIGN, ADD_ASSIGN, SUBTRACT_ASSIGN,
    MULTIPLY_ASSIGN, POWER_ASSIGN,
    DIVIDE_ASSIGN, AND_ASSIGN,
    OR_ASSIGN, XOR_ASSIGN,
    SHIFT_LEFT_ASSIGN, SHIFT_RIGHT_ASSIGN,
    ROTATE_LEFT_ASSIGN, ROTATE_RIGHT_ASSIGN,
    CONCATENATE_ASSIGN, BIT_SET_ASSIGN,
    BIT_CLEAR_ASSIGN, BIT_TOGGLE_ASSIGN,

    COMMA, TILDE, SEMICOLON, /*COLON,*/
    LEFT_PAREN, PAREN_RIGHT,
    BRACE_LEFT, BRACE_RIGHT,
    BRACKET_LEFT, BRACKET_RIGHT,
    /*THAN_LEFT, THAN_RIGHT*/
};

static const trie<Keyword> string_to_keyword {
    {"void", Keyword::VOID}, {"byte", Keyword::BYTE}, {"char", Keyword::CHAR},
    {"short", Keyword::SHORT}, {"int", Keyword::INT}, {"long", Keyword::LONG},
    {"sig", Keyword::SIG}, {"float", Keyword::FLOAT}, {"double", Keyword::DOUBLE},
    {"bool", Keyword::BOOL}, {"struct", Keyword::STRUCT}, {"union", Keyword::UNION},
    {"enum", Keyword::ENUM}, {"string", Keyword::STRING}, {"fun", Keyword::FUN},
    {"let", Keyword::LET},

    {"const", Keyword::CONST}, {"volatile", Keyword::VOLATILE},

    {"if", Keyword::IF}, {"else", Keyword::ELSE}, {"for", Keyword::FOR},
    {"while", Keyword::WHILE}, {"do", Keyword::DO}, {"switch", Keyword::SWITCH},
    {"return", Keyword::RETURN}, {"raise", Keyword::RAISE}, {"break", Keyword::BREAK},
    {"continue", Keyword::CONTINUE}, {"leave", Keyword::LEAVE}, {"goto", Keyword::GOTO},
    {"label", Keyword::LABEL}, {"defer", Keyword::DEFER},

    {"true", Keyword::TRUE}, {"false", Keyword::FALSE}, {"null", Keyword::_NULL},
    {"import", Keyword::IMPORT}, {"using", Keyword::USING}, {"namespace", Keyword::NAMESPACE},
    {"case", Keyword::CASE}, {"as", Keyword::AS}, {"new", Keyword::NEW}, {"delete", Keyword::DELETE},

    {"try", Keyword::TRY}, {"catch", Keyword::CATCH}, {"and", Keyword::AND}, {"or", Keyword::OR}
};

static const trie<Symbol>  string_to_symbol {
    {"$", Symbol::COMPILER},
    
    {"*", Symbol::POINTER}, {"*!", Symbol::UNIQUE_POINTER}, {"*?", Symbol::WEAK_POINTER},
    {"*+", Symbol::SHARED_POINTER},
    
    {"---", Symbol::NOTHING},
    
    {"++", Symbol::INCREMENT}, {"--", Symbol::DECREMENT},
    
    {"+", Symbol::ADD}, {"-", Symbol::SUBSTRACT},/*{"*", Symbol::MULTIPLY},*/
    {"**", Symbol::POWER}, {"%", Symbol::MODULO},
    
    /*{"*", Symbol::ADDRESS},*/{"@", Symbol::DEREFERENCE},
    
    {"!", Symbol::NOT}, {"&", Symbol::AND}, {"|", Symbol::OR}, {"^", Symbol::XOR},
    
    {"!!", Symbol::LNOT}, {"&&", Symbol::LAND}, {"||", Symbol::LOR}, {"^^", Symbol::LXOR},
    
    {".", Symbol::ACCESS}, {"..", Symbol::CONCATENATE}, {"...", Symbol::SPREAD},
    
    {"?", Symbol::TERNARY_CHOICE}, {":", Symbol::TERNARY_CONDITION},
    
    {"@|", Symbol::BIT_SET}, {"@&", Symbol::BIT_CLEAR}, {"@?", Symbol::BIT_CHECK}, {"@^", Symbol::BIT_TOGGLE},
    
    {"<<", Symbol::SHIFT_LEFT}, {">>", Symbol::SHIFT_RIGHT}, {"<<<", Symbol::ROTATE_LEFT},
    {">>>", Symbol::ROTATE_RIGHT},
    
    {"==", Symbol::EQUALS}, {"!=", Symbol::NOT_EQUALS}, {">", Symbol::GREATER},
    {">=", Symbol::GREATER_OR_EQUALS}, {"<", Symbol::LESS}, {"<=", Symbol::LESS_OR_EQUALS},
    
    {"=", Symbol::ASSIGN}, {"+=", Symbol::ADD_ASSIGN}, {"-=", Symbol::SUBTRACT_ASSIGN},
    {"*=", Symbol::MULTIPLY_ASSIGN}, {"**=", Symbol::POWER_ASSIGN}, {"/=", Symbol::DIVIDE_ASSIGN},
    {"&=", Symbol::AND_ASSIGN}, {"|=", Symbol::OR_ASSIGN}, {"^=", Symbol::XOR_ASSIGN},
    {"<<=", Symbol::SHIFT_LEFT_ASSIGN}, {">>=", Symbol::SHIFT_RIGHT_ASSIGN}, {"<<<=", Symbol::ROTATE_LEFT_ASSIGN},
    {">>>=", Symbol::ROTATE_RIGHT_ASSIGN}, {"..=", Symbol::CONCATENATE_ASSIGN}, {"@|=", Symbol::BIT_SET_ASSIGN},
    {"@&=", Symbol::BIT_CLEAR_ASSIGN}, {"@^=", Symbol::BIT_TOGGLE_ASSIGN},
    
    {",", Symbol::COMMA}, {"~", Symbol::TILDE},{";", Symbol::SEMICOLON},
    /*{":", Symbol::COLON},*/{"(", Symbol::LEFT_PAREN}, {")", Symbol::PAREN_RIGHT},
    {"{", Symbol::BRACE_LEFT}, {"}", Symbol::BRACE_RIGHT}, {"[", Symbol::BRACKET_LEFT},
    {"]", Symbol::BRACKET_RIGHT},/*{"<", Symbol::THAN_LEFT},*//*{">", Symbol::THAN_RIGHT},*/
};

}