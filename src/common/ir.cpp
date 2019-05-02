#include "common/ir.h"
#include <sstream>
#include "ast.h"
#include "symbol_table.h"
#include "type.h"
#include <iomanip>

ir_triple::ir_triple_param::ir_triple_param(ast* node) {
    if (node && node->is_symbol()) {
        iden = node->as_symbol().symbol;
        type = IDEN;
    } else {
        value = node;
        type = LITERAL;
    }
}
ir_triple::ir_triple_param::ir_triple_param(st_entry* entry) 
    : iden(entry), type(IDEN) {};
ir_triple::ir_triple_param::ir_triple_param(ir_triple* triple)
    : triple(triple), type(TRIPLE) {};
ir_triple::ir_triple_param::ir_triple_param(u64 immediate)
    : immediate(immediate), type(IMMEDIATE) {};
    
void ir_triple::set_label(const std::string& str) {
#ifdef DEBUG
    label = str;
#endif
}
    
std::string ir_triple::print() {
    std::stringstream ss{};
    ss << op;
    if (param1.value || param1.type == ir_triple_param::IMMEDIATE) {
        ss << " | ";
        if (param1.type == ir_triple_param::LITERAL) {
            ss << param1.value->print_value();
        } else if (param1.type == ir_triple_param::IDEN) {
            ss << "IDEN " << param1.iden->name << " (" << param1.iden->get_type() << ")";
        } else if (param1.type == ir_triple_param::TRIPLE) {
            ss << "TRIPLE ()";
        } else {
            ss << "IMMEDIATE " << param1.immediate;
        }
    }
    if (param2.value || param2.type == ir_triple_param::IMMEDIATE) {
        ss << " | ";
        if (param2.type == ir_triple_param::LITERAL) {
            ss << param2.value->print_value();
        } else if (param2.type == ir_triple_param::IDEN) {
            ss << "IDEN " << param2.iden->name << " (" << param2.iden->get_type() << ")";
        } else if (param2.type == ir_triple_param::TRIPLE) {
            ss << "TRIPLE ()";
        } else {
            ss << "IMMEDIATE " << param2.immediate;
        }
    }
    return ss.str();
}

std::string ir_triple::print(const std::map<ir_triple*, u64>& triples) {
    std::stringstream ss{};
    ss << op;
    if (param1.value || param1.type == ir_triple_param::IMMEDIATE) {
        ss << " | ";
        if (param1.type == ir_triple_param::LITERAL) {
            ss << param1.value->print_value();
        } else if (param1.type == ir_triple_param::IDEN) {
            ss << "IDEN " << param1.iden->name << " (" << param1.iden->get_type() << ")";
        } else if (param1.type == ir_triple_param::TRIPLE) {
            ss << "TRIPLE ";
            if (auto id = triples.find(param1.triple); id != triples.end()) {
                ss << id->second;
#ifdef DEBUG
                if (!id->first->label.empty()) {
                    ss << " \"" << id->first->label << "\"";
                }
#endif
            } else {
                ss << "???";
            }
        } else {
            ss << "IMMEDIATE " << param1.immediate;
        }
    }
    if (param2.value || param2.type == ir_triple_param::IMMEDIATE) {
        ss << " | ";
        if (param2.type == ir_triple_param::LITERAL) {
            ss << param2.value->print_value();
        } else if (param2.type == ir_triple_param::IDEN) {
            ss << "IDEN " << param2.iden->name << " (" << param2.iden->get_type() << ")";
        } else if (param2.type == ir_triple_param::TRIPLE) {
            ss << "TRIPLE ";
            if (auto id = triples.find(param2.triple); id != triples.end()) {
                ss << id->second;
#ifdef DEBUG
                if (!id->first->label.empty()) {
                    ss << " \"" << id->first->label << "\"";
                }
#endif
            } else {
                ss << "???";
            }
        } else {
            ss << "IMMEDIATE " << param2.immediate;
        }
    }
#ifdef DEBUG
    if (!label.empty()) {
        ss << " -> \"" << label << "\"";
    }
#endif
    return ss.str();
}

void ir_triple_range::append(ir_triple* triple) {
    end->next = triple;
    end = triple;
}

void ir_triple_range::prepend(ir_triple* triple) {
    triple->next = start;
    start = triple;
}

block::block(ir_triple_range begin) {
    start = begin;
}

void block::add(ir_triple_range begin) {
    start.end->next = begin.start;
    start.end = begin.end;
}

void block::add(ir_triple* triple) {
    start.append(triple);
}

void block::add_end(ir_triple_range finish) {
    if (end.start) {
        finish.end->next = end.start;
        end.start = finish.start;
    } else {
        end = finish;
    }
}

void block::add_end(ir_triple* triple) {
    if (end.start) {
        end.prepend(triple);
    } else {
        end = {triple, triple};
    }
}

void block::finish() {
    if (end.start) {
        start.end->next = end.start;
        start.end = end.end;
        end = start;
    } else {
        end = start;
    }
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
        case NOOP:
            return os << "NOOP";
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
}
