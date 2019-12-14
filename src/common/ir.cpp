#include "common/ir.h"
#include <sstream>
#include "ast.h"
#include "symbol_table.h"
#include "type.h"
#include <iomanip>

std::ostream& operator<<(std::ostream& os, const ir_op::code& code) {
    using namespace ir_op;
    switch (code) {
        case ADD:
            return os << "ADD";
        case SUBTRACT:
            return os << "SUBTRACT";
        case MULTIPLY:
            return os << "MULTIPLY";
        case DIVIDE:
            return os << "DIVIDE";
        case POWER:
            return os << "POWER";
        case MODULO:
            return os << "MODULO";
        case INCREMENT:
            return os << "INCREMENT";
        case DECREMENT:
            return os << "DECREMENT";
        case NEGATE:
            return os << "NEGATE";
        case SHIFT_LEFT:
            return os << "SHIFT_LEFT";
        case SHIFT_RIGHT:
            return os << "SHIFT_RIGHT";
        case ROTATE_LEFT:
            return os << "ROTATE_LEFT";
        case ROTATE_RIGHT:
            return os << "ROTATE_RIGHT";
        case AND:
            return os << "AND";
        case OR:
            return os << "OR";
        case XOR:
            return os << "XOR";
        case NOT:
            return os << "NOT";
        case CONCATENATE:
            return os << "CONCATENATE";
        case CAST_FTD:
            return os << "CAST_FLOAT_TO_DOUBLE";
        case CAST_DTF:
            return os << "CAST_DOUBLE_TO_FLOAT";
        case CAST_STU:
            return os << "CAST_SIGNED_TO_UNSIGNED";
        case CAST_UTS:
            return os << "CAST_UNSIGNED_TO_SIGNED";
        case CAST_UTF:
            return os << "CAST_UNSIGNED_TO_FLOAT";
        case CAST_STF:
            return os << "CAST_SIGNED_TO_FLOAT";
        case CAST_UTD:
            return os << "CAST_UNSIGNED_TO_DOUBLE";
        case CAST_STD:
            return os << "CAST_SIGNED_TO_DOUBLE";
        case CAST_FTU:
            return os << "CAST_FLOAT_TO_UNSIGNED";
        case CAST_FTS:
            return os << "CAST_FLOAT_TO_SIGNED";
        case CAST_DTU:
            return os << "CAST_DOUBLE_TO_UNSIGNED";
        case CAST_DTS:
            return os << "CAST_DOUBLE_TO_SIGNED";
        case LESS:
            return os << "LESS";
        case LESS_EQUALS:
            return os << "LESS_EQUALS";
        case GREATER:
            return os << "GREATER";
        case GREATER_EQUALS:
            return os << "GREATER_EQUALS";
        case EQUALS:
            return os << "EQUALS";
        case NOT_EQUALS:
            return os << "NOT_EQUALS";
        case BIT_SET:
            return os << "BIT_SET";
        case BIT_NOT_SET:
            return os << "BIT_NOT_SET";
        case JUMP:
            return os << "JUMP";
        case SYMBOL:
            return os << "SYMBOL";
        case VALUE:
            return os << "VALUE";
        case TEMP:
            return os << "TEMP";
        case IF_FALSE:
            return os << "IF_FALSE";
        case IF_TRUE:
            return os << "IF_TRUE";
        case CALL:
            return os << "CALL";
        case CALL_CLOSURE:
            return os << "CALL_CLOSURE";
        case PARAM:
            return os << "PARAM";
        case RETURN:
            return os << "RETURN";
        case RETVAL:
            return os << "RETVAL";
        case RETPUSH:
            return os << "RETPUSH";
        case NEW:
            return os << "NEW";
        case DELETE:
            return os << "DELETE";
        case COPY:
            return os << "COPY";
        case INDEX:
            return os << "INDEX";
        case OFFSET:
            return os << "OFFSET";
        case ADDRESS:
            return os << "ADDRESS";
        case DEREFERENCE:
            return os << "DEREFERENCE";
        case LENGTH:
            return os << "LENGTH";
        case ZERO:
            return os << "ZERO";
        case NOOP:
            return os << "NOOP";
        default:
            return os << "INVALID";
    }
}
/*
std::string print_sequence(ir_triple* start) {
    ir_triple* cur = start;
    std::stringstream ss{};
    std::map<ir_triple*, u64> triples{};
    u64 i = 0;
    while (cur) {
        triples.insert({cur, i++});
        cur = cur->next;
    }
    
    cur = start;
    i = 0;
    while (cur) {
        ss << std::setw(10) << i++ << ": " << cur->print(triples) << '\n';
        cur = cur->next;
    }
    return ss.str();
}*/
