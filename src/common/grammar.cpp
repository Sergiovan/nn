#include "common/grammar.h"
#include "common/util.h"

using namespace grammar;

dict<std::string, u64> grammar::string_to_symbol {
    {"u0", (u64) symbol::KW_U0},
    {"u8", (u64) symbol::KW_U8},
    {"u16", (u64) symbol::KW_U16},
    {"u32", (u64) symbol::KW_U32},
    {"u64", (u64) symbol::KW_U64},
    {"s8", (u64) symbol::KW_S8},
    {"s16", (u64) symbol::KW_S16},
    {"s32", (u64) symbol::KW_S32},
    {"s64", (u64) symbol::KW_S64},
    {"u1", (u64) symbol::KW_U1},
    {"e64", (u64) symbol::KW_E64},
    {"f32", (u64) symbol::KW_F32},
    {"f64", (u64) symbol::KW_F64},
    {"c8", (u64) symbol::KW_C8},
    {"c16", (u64) symbol::KW_C16},
    {"c32", (u64) symbol::KW_C32},
    {"type", (u64) symbol::KW_TYPE},
    {"any", (u64) symbol::KW_ANY},
    {"fun", (u64) symbol::KW_FUN},
    {"struct", (u64) symbol::KW_STRUCT},
    {"union", (u64) symbol::KW_UNION},
    {"enum", (u64) symbol::KW_ENUM},
    {"tuple", (u64) symbol::KW_TUPLE},
    
    {"infer", (u64) symbol::KW_INFER},
    {"var", (u64) symbol::KW_VAR},
    {"let", (u64) symbol::KW_LET},
    {"ref", (u64) symbol::KW_REF},
    {"def", (u64) symbol::KW_DEF},
    {"const", (u64) symbol::KW_CONST},
    {"volat", (u64) symbol::KW_VOLAT},
    
    {"if", (u64) symbol::KW_IF},
    {"else", (u64) symbol::KW_ELSE},
    {"for", (u64) symbol::KW_FOR},
    {"loop", (u64) symbol::KW_LOOP},
    {"while", (u64) symbol::KW_WHILE},
    {"do", (u64) symbol::KW_DO},
    {"switch", (u64) symbol::KW_SWITCH},
    {"case", (u64) symbol::KW_CASE},
    {"return", (u64) symbol::KW_RETURN},
    {"raise", (u64) symbol::KW_RAISE},
    {"break", (u64) symbol::KW_BREAK},
    {"continue", (u64) symbol::KW_CONTINUE},
    {"goto", (u64) symbol::KW_GOTO},
    {"label", (u64) symbol::KW_LABEL},
    {"defer", (u64) symbol::KW_DEFER},
    
    {"try", (u64) symbol::KW_TRY},
    {"catch", (u64) symbol::KW_CATCH},
    
    {"true", (u64) symbol::KW_TRUE},
    {"false", (u64) symbol::KW_FALSE},
    {"null", (u64) symbol::KW_NULL},
    
    {"import", (u64) symbol::KW_IMPORT},
    {"using", (u64) symbol::KW_USING},
    {"namespace", (u64) symbol::KW_NAMESPACE},
    {"as", (u64) symbol::KW_AS},
    {"in", (u64) symbol::KW_IN},
    {"new", (u64) symbol::KW_NEW},
    {"delete", (u64) symbol::KW_DELETE},
    {"this", (u64) symbol::KW_THIS},
    
    {"_", (u64) symbol::KW_PLACEHOLDER},
    {"sizeof", (u64) symbol::KW_SIZEOF},
    {"typeof", (u64) symbol::KW_TYPEOF},
    {"typeinfo", (u64) symbol::KW_TYPEINFO},

    {"yield", (u64) symbol::KW_YIELD},
    {"match", (u64) symbol::KW_MATCH},
    {"dynamic", (u64) symbol::KW_DYNAMIC},
    {"static", (u64) symbol::KW_STATIC},
    {"and", (u64) symbol::KW_AND},
    {"or", (u64) symbol::KW_OR},
    {"asm", (u64) symbol::KW_ASM},

    {"+", (u64) symbol::ADD},
    {"-", (u64) symbol::SUB},
    {"*", (u64) symbol::MUL},
    {"@", (u64) symbol::AT},
    {"/", (u64) symbol::DIV},
    {"//", (u64) symbol::IDIV},
    {"%", (u64) symbol::MODULO},
    {"..", (u64) symbol::CONCAT},
    {">>", (u64) symbol::SHL},
    {"<<", (u64) symbol::SHR},
    {">>>", (u64) symbol::RTL},
    {"<<<", (u64) symbol::RTR},
    {"<<|", (u64) symbol::BITSET},
    {"<<&", (u64) symbol::BITCLEAR},
    {"<<^", (u64) symbol::BITTOGGLE},
    {"<<?", (u64) symbol::BITCHECK},
    {"&", (u64) symbol::AND},
    {"&&", (u64) symbol::LAND},
    {"|", (u64) symbol::OR},
    {"||", (u64) symbol::LOR},
    {"^", (u64) symbol::XOR},

    {"!", (u64) symbol::NOT},
    {"!!", (u64) symbol::LNOT},
    {"<", (u64) symbol::LT},
    {">", (u64) symbol::GT},
    {"<=", (u64) symbol::LE},
    {">=", (u64) symbol::GE},
    {"==", (u64) symbol::EQUALS},
    {"!=", (u64) symbol::NEQUALS},

    {"?", (u64) symbol::WEAK_PTR},
    {"++", (u64) symbol::INCREMENT},
    {"--", (u64) symbol::DECREMENT},
    {"::[", (u64) symbol::OSELECT},
    {"~", (u64) symbol::LENGTH},


    {":", (u64) symbol::COLON},
    {"::", (u64) symbol::DCOLON},
    {"??", (u64) symbol::DQUESTION},
    {";", (u64) symbol::SEMICOLON},
    {",", (u64) symbol::COMMA},
    {".", (u64) symbol::PERIOD},
    {"=", (u64) symbol::ASSIGN},

    {"^=", (u64) symbol::XOR_ASSIGN},
    {"|=", (u64) symbol::OR_ASSIGN},
    {"&=", (u64) symbol::AND_ASSIGN},
    {"<<|=", (u64) symbol::BITSET_ASSIGN},
    {"<<&=", (u64) symbol::BITCLEAR_ASSIGN},
    {"<<^=", (u64) symbol::BITTOGGLE_ASSIGN},
    {"<<=", (u64) symbol::SHL_ASSIGN},
    {">>=", (u64) symbol::SHR_ASSIGN},
    {"<<<=", (u64) symbol::RTL_ASSIGN},
    {">>>=", (u64) symbol::RTR_ASSIGN},
    {"+=", (u64) symbol::ADD_ASSIGN},
    {"-=", (u64) symbol::SUB_ASSIGN},
    {"..=", (u64) symbol::CONCAT_ASSIGN},
    {"*=", (u64) symbol::MUL_ASSIGN},
    {"/=", (u64) symbol::DIV_ASSIGN},
    {"//=", (u64) symbol::IDIV_ASSIGN},
    {"%=", (u64) symbol::MODULO_ASSIGN},
    
    
    {"'(", (u64) symbol::LITERAL_TUPLE}, 
    {"'[", (u64) symbol::LITERAL_ARRAY}, 
    {"'{", (u64) symbol::LITERAL_STRUCT},
    
    {"(", (u64) symbol::OPAREN},
    {")", (u64) symbol::CPAREN},
    {"{", (u64) symbol::OBRACE},
    {"}", (u64) symbol::CBRACE},
    {"[", (u64) symbol::OBRACK},
    {"]", (u64) symbol::CBRACK},
    {"->", (u64) symbol::RARROW},
    {"=>", (u64) symbol::SRARROW},
    {"...", (u64) symbol::SPREAD},
    {"<>", (u64) symbol::DIAMOND},
    {"---", (u64) symbol::NOTHING},
    
    {"<#", (u64) symbol::COMMENT_CLOSE}
};

dict<u64, std::string> grammar::symbol_to_string {swap_map(string_to_symbol)};

std::ostream& operator<<(std::ostream& os, const symbol& sym) {
    if (auto it = symbol_to_string.find((u64) sym); it != symbol_to_string.end()) {
        return os << it->second;
    } else {
        switch (sym) {
            case grammar::SPECIAL_INVALID:
                return os << "INVALID";
            case grammar::SP_NAMED:
                return os << "NAMED PARAMETER";
            case grammar::SP_PARAM:
                return os << "PARAMETER";
            case grammar::SP_COND:
                return os << "CONDITIONALS";
            case grammar::SP_TYPE:
                return os << "TYPE";
            case grammar::SP_TYPELIT:
                return os << "TYPE LITERAL";
            case grammar::SP_CAPTURE:
                return os << "CAPTURE GROUP";
            case grammar::SP_FUNTYPE:
                return os << "FUNCTION TYPE";
            case grammar::SPECIAL_LAST: [[fallthrough]];
            default:
                return os << "ERROR ERROR ERROR";
        }
    }
}
