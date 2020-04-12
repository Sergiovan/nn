#pragma once

#include <unordered_map>
#include <string>
#include <iostream>

#include "common/defs.h"

namespace grammar {
    
    enum symbol : u64 {
        SPECIAL_INVALID,
        
        KW_U0, KW_U8, KW_U16, KW_U32, KW_U64,
        KW_S8, KW_S16, KW_S32, KW_S64, KW_U1,
        KW_E64, KW_F32, KW_F64, KW_C8, KW_C16,
        KW_C32, KW_TYPE,
        KW_ANY, KW_FUN, KW_STRUCT, KW_UNION,
        KW_ENUM, KW_TUPLE, 
        KW_INFER, KW_VAR, KW_LET, KW_REF, KW_DEF, KW_CONST,
        KW_VOLAT, 
        
        KW_IF, KW_ELSE, KW_FOR, KW_LOOP, KW_WHILE, 
        KW_DO, KW_SWITCH, KW_CASE, KW_RETURN, KW_RAISE,
        KW_BREAK, KW_CONTINUE, KW_GOTO, KW_LABEL, KW_DEFER, 
        KW_TRY, KW_CATCH,
        
        KW_TRUE, KW_FALSE, KW_NULL,
        
        KW_IMPORT, KW_USING, KW_NAMESPACE,
        KW_AS, KW_IN, KW_NEW, KW_DELETE, KW_THIS, 
        KW_PLACEHOLDER, KW_SIZEOF, KW_TYPEOF, KW_TYPEINFO,
        
        KW_YIELD, KW_MATCH, KW_DYNAMIC, KW_STATIC, KW_AND, KW_OR, 
        
        
        ADD, SHARED_PTR = ADD, 
        SUB, 
        MUL, POINTER = MUL, ADDRESSOF = MUL, 
        AT, 
        DIV, IDIV, MODULO,
        CONCAT,
        SHL, SHR, RTL, RTR,
        BITSET, BITCLEAR, BITTOGGLE, BITCHECK,
        AND, LAND,
        OR, LOR,
        XOR, 
        NOT, UNIQUE_PTR = NOT, LNOT,
        LT, GT, LE, GE,
        EQUALS, NEQUALS, 
        WEAK_PTR,
        INCREMENT, DECREMENT, OSELECT,
        LENGTH,
        
        COLON, CHOICE = COLON, DCOLON, DQUESTION, SEMICOLON, COMMA, PERIOD, 
        
        ASSIGN, XOR_ASSIGN, OR_ASSIGN, AND_ASSIGN, 
        BITSET_ASSIGN, BITCLEAR_ASSIGN, BITTOGGLE_ASSIGN,
        SHL_ASSIGN, SHR_ASSIGN, RTL_ASSIGN, RTR_ASSIGN,
        ADD_ASSIGN, SUB_ASSIGN, CONCAT_ASSIGN, MUL_ASSIGN,
        DIV_ASSIGN, IDIV_ASSIGN, MODULO_ASSIGN, 
        
        LITERAL_TUPLE, LITERAL_ARRAY, LITERAL_STRUCT, LITERAL_FUNCTION,
        
        OPAREN, CPAREN, OBRACE, CBRACE, OBRACK, CBRACK,
        RARROW, SRARROW, SPREAD, DIAMOND,
        NOTHING,
        
        COMMENT_CLOSE,
        
        SP_NAMED, SP_PARAM, SP_COND, SP_TYPE, SP_TYPELIT, SP_CAPTURE, SP_FUNTYPE,
        
        SPECIAL_LAST
    };
    
    [[maybe_unused]]
    extern dict<std::string, u64> string_to_symbol;
    extern dict<u64, std::string> symbol_to_string;
    
}

std::ostream& operator<<(std::ostream& os, const grammar::symbol& sym);
