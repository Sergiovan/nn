#include "common/ir.h"

#include <sstream>
#include <iomanip>

#include "ast.h"
#include "symbol_table.h"
#include "type.h"
#include "utils.h"

ir_triple_param::ir_triple_param(u64 immediate) : immediate(immediate), type(IMMEDIATE) {}
ir_triple_param::ir_triple_param(s32 immediate) : immediate(immediate), type(IMMEDIATE) {}
ir_triple_param::ir_triple_param(ast* value) {
    if (value && value->is_symbol()) {
        type = IDEN;
        iden = value->as_symbol().symbol;
    } else {
        type = VALUE;
        ir_triple_param::value = value;
    }
}
ir_triple_param::ir_triple_param(st_entry* iden) : iden(iden), type(IDEN) {}
ir_triple_param::ir_triple_param(ir_triple* triple) : triple(triple), type(TRIPLE) {}
ir_triple_param::ir_triple_param(std::nullptr_t) : ir_triple_param((ast*) nullptr) {}

ir_triple_param::operator ast*() const {
    if (type == VALUE) return value;
    return nullptr;
}

ir_triple_param::operator st_entry*() const {
    if (type == IDEN) return iden;
    return nullptr;
}

ir_triple_param::operator ir_triple*() const {
    if (type == TRIPLE) return triple;
    return nullptr;
}

ir_triple_param::operator u64() const {
    if (type == IMMEDIATE) return immediate;
    return 0;
}

void ir_triple_param::set(u64 immediate) {
    ir_triple_param::immediate = immediate;
    type = IMMEDIATE;
}

void ir_triple_param::set(s32 immediate) {
    ir_triple_param::immediate = immediate;
    type = IMMEDIATE;
}

void ir_triple_param::set(ast* value) {
    ir_triple_param::value = value;
    type = VALUE;
}

void ir_triple_param::set(st_entry* iden) {
    ir_triple_param::iden = iden;
    type = IDEN;
}

void ir_triple_param::set(ir_triple* triple) {
    ir_triple_param::triple = triple;
    type = TRIPLE;
}

void ir_triple_param::set(std::nullptr_t) {
    set((ast*) nullptr);
}

std::string ir_triple_param::print() {
    std::stringstream ss{};
    
    switch (type) {
        case VALUE:
            ss << "VALUE";
            if (value->is_value()) {
                ss << " " << value->print_value();
            }
            break;
        case IDEN: {
            ss << "IDEN " << iden->name;
            break;
        }
        case TRIPLE: {
            ss << "TRIPLE " << triple->id;
            if (!triple->label.empty()) {
                ss << " \"" << triple->label << "\"";
            }
            break;
        }
        case IMMEDIATE: {
            ss << "IMMEDIATE 0x" << std::hex << std::uppercase << immediate;
            break;
        }
    }
    
    return ss.str();
}

std::string ir_triple::print() {
    std::stringstream ss{};
    
    if (!label.empty()) {
        ss << logger::color::yellow << "         #  " << label << ":\n" << logger::color::white;
    }
    
    ss << std::setw(10) << id << ": " << op;

    if (op1.type == ir_triple_param::IMMEDIATE || op1.value != nullptr) {
        ss << " | " << op1.print();
    }
    
    if (op2.type == ir_triple_param::IMMEDIATE || op2.value != nullptr) {
        ss << " || " << op2.print();
    }
    
    if (res_type) {
        ss << " (" << res_type << ")";
    }
    
    if (cond) {
        ss << " ---> Jump to TRIPLE " << cond->id;
        if (!cond->label.empty()) {
            ss << " \"" << cond->label << "\"";
        }
    }
    
    return ss.str();
}

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
        case SET_BIT:
            return os << "SET_BIT";
        case CLEAR_BIT:
            return os << "CLEAR_BIT";
        case TOGGLE_BIT:
            return os << "TOGGLE_BIT";
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
        case JUMP_RELATIVE:
            return os << "JUMP_RELATIVE";
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
        case CALL_BTIN:
            return os << "CALL_BTIN";
        case PARAM:
            return os << "PARAM";
        case RETURN:
            return os << "RETURN";
        case RETVAL:
            return os << "RETVAL";
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
        case FUNCTION_START:
            return os << "FUNCTION_START";
        case FUNCTION_END:
            return os << "FUNCTION_END";
        case BLOCK_START:
            return os << "BLOCK_START";
        case BLOCK_END:
            return os << "BLOCK_END";
        default:
            return os << "INVALID";
    }
}

std::string print_sequence(ir_triple* start) {
    ir_triple* cur = start;
    std::stringstream ss{};
    std::map<ir_triple*, u64> triples{};
    u64 i = 0;
    while (cur) {
        if (cur->id) {
            logger::debug() << cur->print() << logger::nend;
            return {"Error"};
        }
        cur->id = i++;
        cur = cur->next;
    }
    
    cur = start;
    i = 0;
    while (cur) {
        ss << cur->print() << '\n';
        cur = cur->next;
    }
    return ss.str();
}
