#pragma once

#include <map>
#include <string>

#include "common/convenience.h"

namespace Grammar {

enum class TokenType {
    KEYWORD,
    SYMBOL,
    IDENTIFIER,
    COMPILER_IDENTIFIER,
    NUMBER,
    STRING,
    CHARACTER,
    END_OF_FILE,
    NONE
};

enum class Keyword : char {
    /* Types */
    VOID = 0, BYTE = 1, SHORT = 2, INT = 3,
    LONG = 4, SIG = 11, FLOAT = 5, DOUBLE = 6, BOOL = 10,
    CHAR = 8, STRING = 9, FUN = 12, LET = 13,

    STRUCT, UNION, ENUM,

    /* Type modifiers */
    CONST, VOLATILE, SIGNED, UNSIGNED,

    /* Control */
    IF, ELSE, FOR, WHILE,
    DO, SWITCH, RETURN,
    RAISE, BREAK, CONTINUE,
    LEAVE, GOTO, LABEL, DEFER,
    TRY, CATCH,

    /* Other */
    TRUE, FALSE, NNULL, IMPORT, 
    USING, NAMESPACE, CASE, AS, 
    NEW, DELETE, THIS,

    /* Reserved */
    AND, OR, TYPEOF, SIZEOF,
    
    /* Invalid */
    KEYWORD_INVALID
};

enum class Symbol : char {
    POINTER = 0, MULTIPLY = 0, ADDRESS = 0,
    GREATER = 1, LT = 1,
    LESS = 2, GT = 2,
    TERNARY_CHOICE = 3, COLON = 3,
    NOT = 4, EMARK = 4,
    ACCESS = 5, DOT = 5, 
    
    COMPILER,
    
    /*POINTER,*/ UNIQUE_POINTER,
    WEAK_POINTER, SHARED_POINTER,

    NOTHING,

    INCREMENT, DECREMENT,

    ADD, SUBTRACT, /*MULTIPLY,*/ POWER,
    DIVIDE, MODULO,

    /*ADDRESS,*/ AT,

    /*NOT,*/ AND, OR, XOR,

    LNOT, LAND, LOR, LXOR,

    /*ACCESS,*/ CONCATENATE, SPREAD,

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
    /*EMARK,*/ PAREN_LEFT, PAREN_RIGHT,
    BRACE_LEFT, BRACE_RIGHT,
    BRACKET_LEFT, BRACKET_RIGHT,
    /*DOT, THAN_LEFT, THAN_RIGHT,*/
    
    /* NOT SYMBOLS, JUST FOR AST PURPOSES */
    KWIF, KWELSE, KWFOR, KWFORCLASSIC, KWFOREACH, KWFORLUA,
    KWWHILE, KWSWITCH,
    KWRETURN, KWRAISE, KWGOTO, KWLABEL,
    KWTRY, KWCATCH, KWCASE,
    KWDEFER, KWBREAK, KWCONTINUE, KWLEAVE, KWIMPORT,
    KWUSING, KWNAMESPACE, 
    FUN_CALL, FUN_RET,

    SYMDECL,
    
    /* NOT REALLY A SYMBOL OR FOR AST PURPOSES */
    COMMENT, COMMENT_START, COMMENT_END,
    
    SYMBOL_INVALID
};

static const std::map<TokenType, std::string> tokentype_names {
    {TokenType::KEYWORD,               "Keyword"},
    {TokenType::SYMBOL,                "Symbol"},
    {TokenType::IDENTIFIER,            "Identifier"},
    {TokenType::COMPILER_IDENTIFIER,   "$Identifier"},
    {TokenType::NUMBER,                "Number"},
    {TokenType::STRING,                "String"},
    {TokenType::CHARACTER,             "Character"},
    {TokenType::END_OF_FILE,           "EOF"},
    {TokenType::NONE,                  "Invalid"}
};

static const dict<Keyword> string_to_keyword {
    {"void", Keyword::VOID}, {"byte", Keyword::BYTE}, {"char", Keyword::CHAR},
    {"short", Keyword::SHORT}, {"int", Keyword::INT}, {"long", Keyword::LONG},
    {"sig", Keyword::SIG}, {"float", Keyword::FLOAT}, {"double", Keyword::DOUBLE},
    {"bool", Keyword::BOOL}, {"struct", Keyword::STRUCT}, {"union", Keyword::UNION},
    {"enum", Keyword::ENUM}, {"string", Keyword::STRING}, {"fun", Keyword::FUN},
    {"let", Keyword::LET},

    {"const", Keyword::CONST}, {"volatile", Keyword::VOLATILE}, {"signed", Keyword::SIGNED},
    {"unsigned", Keyword::UNSIGNED},

    {"if", Keyword::IF}, {"else", Keyword::ELSE}, {"for", Keyword::FOR},
    {"while", Keyword::WHILE}, {"do", Keyword::DO}, {"switch", Keyword::SWITCH},
    {"return", Keyword::RETURN}, {"raise", Keyword::RAISE}, {"break", Keyword::BREAK},
    {"continue", Keyword::CONTINUE}, {"leave", Keyword::LEAVE}, {"goto", Keyword::GOTO},
    {"label", Keyword::LABEL}, {"defer", Keyword::DEFER}, {"try", Keyword::TRY}, 
    {"catch", Keyword::CATCH},

    {"true", Keyword::TRUE}, {"false", Keyword::FALSE}, {"null", Keyword::NNULL},
    {"import", Keyword::IMPORT}, {"using", Keyword::USING}, {"namespace", Keyword::NAMESPACE},
    {"case", Keyword::CASE}, {"as", Keyword::AS}, {"new", Keyword::NEW}, {"delete", Keyword::DELETE},

    {"and", Keyword::AND}, {"or", Keyword::OR}, {"typeof", Keyword::TYPEOF}, {"sizeof", Keyword::SIZEOF},
};

static const dict<Symbol>  string_to_symbol {
    {"$", Symbol::COMPILER},
    
    {"*", Symbol::POINTER}, {"*!", Symbol::UNIQUE_POINTER}, {"*?", Symbol::WEAK_POINTER},
    {"*+", Symbol::SHARED_POINTER},
    
    {"---", Symbol::NOTHING},
    
    {"++", Symbol::INCREMENT}, {"--", Symbol::DECREMENT},
    
    {"+", Symbol::ADD}, {"-", Symbol::SUBTRACT},/*{"*", Symbol::MULTIPLY},*/
    {"**", Symbol::POWER}, {"/", Symbol::DIVIDE}, {"%", Symbol::MODULO},
    
    /*{"*", Symbol::ADDRESS},*/{"@", Symbol::AT},
    
    {"!", Symbol::NOT}, {"&", Symbol::AND}, {"|", Symbol::OR}, {"^", Symbol::XOR},
    
    {"!!", Symbol::LNOT}, {"&&", Symbol::LAND}, {"||", Symbol::LOR}, {"^^", Symbol::LXOR},
    
    {".", Symbol::ACCESS}, {"..", Symbol::CONCATENATE}, {"...", Symbol::SPREAD},
    
    {":", Symbol::TERNARY_CHOICE}, {"?", Symbol::TERNARY_CONDITION},
    
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
    /*{":", Symbol::COLON},*/{"(", Symbol::PAREN_LEFT}, {")", Symbol::PAREN_RIGHT},
    {"{", Symbol::BRACE_LEFT}, {"}", Symbol::BRACE_RIGHT}, {"[", Symbol::BRACKET_LEFT},
    {"]", Symbol::BRACKET_RIGHT},/*{"<", Symbol::THAN_LEFT},*//*{">", Symbol::THAN_RIGHT},*/
    
    {"#", Symbol::COMMENT}, {"#>", Symbol::COMMENT_START}, {"<#", Symbol::COMMENT_END},
};

}