#include "common/ir.h"
#include <sstream>
#include "ast.h"
#include "symbol_table.h"
#include <iomanip>

ir_triple::ir_triple_param::ir_triple_param(ast* node) 
    : value(node), type(LITERAL) {}
ir_triple::ir_triple_param::ir_triple_param(st_entry* entry) 
    : iden(entry), type(IDEN) {};
ir_triple::ir_triple_param::ir_triple_param(ir_triple* triple)
    : triple(triple), type(TRIPLE) {};
ir_triple::ir_triple_param::ir_triple_param(u64 immediate)
    : immediate(immediate), type(IMMEDIATE) {};
    
std::string ir_triple::print() {
    std::stringstream ss{};
    ss << op;
    if (param1.value || param1.type == ir_triple_param::IMMEDIATE) {
        ss << " | ";
        if (param1.type == ir_triple_param::LITERAL) {
            ss << param1.value->print_value();
        } else if (param1.type == ir_triple_param::IDEN) {
            ss << "IDEN " << param1.iden->name;
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
            ss << "IDEN " << param2.iden->name;
        } else if (param2.type == ir_triple_param::TRIPLE) {
            ss << "TRIPLE ()";
        } else {
            ss << "IMMEDIATE " << param2.immediate;
        }
    }
    return ss.str();
}

    
ir::ir() { }

ir::~ir() {
    for (auto ptr : triples) {
        if (ptr) {
            delete ptr;
        } 
    }
}

ir::ir(ir&& o) {
    triples.swap(o.triples);
}

ir& ir::operator=(ir&& o) {
    if (&o != this) {
        triples.swap(o.triples);
    }
    return *this;
}

void ir::add(ir_triple t) {
    triples.push_back(new ir_triple{t});
}

void ir::add(ir_triple t, u64 at) {
    triples.insert(triples.begin() + at, new ir_triple{t});
}

void ir::merge_in(ir&& o) {
    if (o.triples.empty()) {
        return;
    }
    u64 tsize = triples.size();
    triples.insert(triples.end(), o.triples.begin(), o.triples.end());
    o.triples.clear();
}

void ir::merge_in(ir&& o, u64 at) {
    u64 tsize = triples.size();
    triples.insert(triples.begin() + at, o.triples.begin(), o.triples.end());
    o.triples.clear();
}

void ir::move(u64 from, u64 to) {
    ir_triple* elem = triples[from];
    triples.erase(triples.begin() + from);
    triples.insert(triples.begin() + to, elem);
}

std::string ir::print() {
    std::stringstream ss{};
    for (u64 i = 0; i < triples.size(); ++i) {
        ss << (i ? "      " : "") << std::setw(5) << i << ": " << triples[i]->print() << "\n";
    }
    if (triples.empty()) {
        ss << "\n";
    }
    return ss.str();
}

block::block(ir* begin) {
    start = latest = begin;
}

void block::add(ir* new_ir) {
    latest->next = new_ir;
    latest = new_ir;
}

void block::add_end(ir* new_end) {
    ir* old_end = end;
    end = new_end;
    new_end->next = old_end;
}

void block::finish() {
    if (end) {
        ir* temp = latest->next;
        latest->next = end;
        end->next = temp;
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
        case JUMP:
            return os << "JUMP";
        case IF_ZERO:
            return os << "IF_ZERO";
        case IF_NOT_ZERO:
            return os << "IF_NOT_ZERO";
        case IF_LESS_THAN_ZERO:
            return os << "IF_LESS_THAN_ZERO";
        case IF_GREATER_THAN_ZERO:
            return os << "IF_GREATER_THAN_ZERO";
        case IF_BIT_SET:
            return os << "IF_BIT_SET";
        case IF_BIT_NOT_SET:
            return os << "IF_BIT_NOT_SET";
        case CALL:
            return os << "CALL";
        case PARAM:
            return os << "PARAM";
        case RETURN:
            return os << "RETURN";
        case RETVAL:
            return os << "RETVAL";
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

std::string print_sequence(ir* start) {
    std::stringstream ss{};
    ir* cur = start;
    u64 i = 0;
    while (cur) {
        ss << std::setw(5) << i++ << " " << cur->print();
        cur = cur->next;
    }
    return ss.str();
}

