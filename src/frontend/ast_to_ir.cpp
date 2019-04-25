#include "frontend/ast_to_ir.h"

#include "common/ast.h"
#include "common/symbol_table.h"
#include "common/utils.h"

ir_builder::ir_builder::ir_builder(parse_info& p) : p(p) {
    current = base = new ir{};
    current->add({ir_op::NOOP});
    irs.push_back(base);
}

void ir_builder::build(ast* node, symbol_table* sym) {    
    using namespace ir_op;
    
    if (!node) {
        node = p.result;
    }
    if (!sym) {
        sym = p.root_st;
    }
    
    switch (node->t) {
        case east_type::PRE_UNARY: [[fallthrough]];
        case east_type::POST_UNARY: [[fallthrough]];
        case east_type::BINARY: [[fallthrough]];
        case east_type::BLOCK: break;
        default: 
            logger::error() << "Found wrong ast_type at start: " << (int) node->t << logger::nend;
            return;
    }
    
    if (node->is_block()) {
        blocks.emplace(current);
        
        for (auto bnode : node->as_block().stmts) {
            build(bnode, node->as_block().st);
        }
        
        blocks.pop();
    } else if (node->is_binary()) {
        using Grammar::Symbol;
        auto& bin = node->as_binary();
        switch (bin.op) {
            case Symbol::SYMDECL: { // Function, or type
                if (bin.left->is_symbol()) {
                    switch (bin.left->as_symbol().symbol->t) {
                        case est_entry_type::FUNCTION: {
                            auto ir = labeled.find(bin.left->as_symbol().symbol);
                            if (bin.right->is_none()) {
                                if (ir == labeled.end()) {
                                    labeled[bin.left->as_symbol().symbol] = new ::ir{};
                                }
                            } else {
                                if (ir == labeled.end()) {
                                    labeled[bin.left->as_symbol().symbol] = new_ir();
                                } else {
                                    current->next = ir->second;
                                    current = ir->second;
                                }
                                build(bin.right, bin.right->as_block().st);
                            }
                        }
                        case est_entry_type::TYPE: 
                            return;
                        default:
                            logger::error() << "Wrong declaration type: " << (int) node->t << logger::nend;
                    }
                } else {
                    auto& left  = bin.left->as_block();
                    if (bin.right->is_none()) {
                        // Nothing to do
                        return;
                    }
                    auto& right = bin.right->as_block();
                    
                    for (int i = 0; i < left.stmts.size(); ++i) {
                        if (i >= right.stmts.size()) {
                            break;
                        }
                        if (right.stmts[i]->is_value()) {
                            current->add(ir_triple{COPY, left.stmts[i]->as_symbol().symbol, right.stmts[i]});
                        } else {
                            build(right.stmts[i], right.st);
                            current->add(ir_triple{COPY, left.stmts[i]->as_symbol().symbol, current->triples.back()});
                        }
                    }
                }
            }
            default:
                break;
        }
    } else if (node->is_unary()) {
        using Grammar::Symbol;
        auto& un = node->as_unary();
        if (un.post) {
            switch (un.op) {
                case Symbol::KWLABEL: {
                    auto ir = labeled.find(un.node->as_symbol().symbol);
                    if (ir == labeled.end()) {
                        labeled[un.node->as_symbol().symbol] = new_ir();
                    } else {
                        current->next = ir->second;
                        current = ir->second;
                    }
                    break;
                }
            }
        } else {
            // Increment, decrement, add, subtract, length, not, lnot, at, address
            switch (un.op) {
                case Symbol::INCREMENT:
                    add_proper(INCREMENT, sym, un.node);
                    break;
                case Symbol::DECREMENT:
                    add_proper(DECREMENT, sym, un.node);
                    break;
                case Symbol::ADD:
                    add_proper(AND, sym, un.node, un.node);
                    break;
                case Symbol::SUBTRACT:
                    add_proper(NEGATE, sym, un.node);
                    break;
                case Symbol::LENGTH:
                    add_proper(LENGTH, sym, un.node);
                    break;
                case Symbol::NOT:
                    add_proper(NOT, sym, un.node);
                    break;
                case Symbol::LNOT:
                    add_proper(OR, sym, un.node, ast::qword(~1));
                    current->add({NOT, current->triples.back()});
                    break;
                case Symbol::AT:
                    add_proper(DEREFERENCE, sym, un.node);
                    break;
                case Symbol::ADDRESS:
                    add_proper(ADDRESS, sym, un.node);
                    break;
                default:
                    logger::error() << "Wrong symbol type: " << (int) un.op << logger::nend;
            }
        }
    }
}

void ir_builder::optimize() {
    return;
}

void ir_builder::add_proper(ir_op::code code, symbol_table* sym, ast* param1, ast* param2) {
    ir_triple t{code};
    if (param1->is_symbol()) {
        t.param1 = param1->as_symbol().symbol;
    } else if (param1->is_value()) {
        t.param1 = param1;
    } else {
        build(param1, sym);
        t.param1 = current->triples.back();
    }
    if (param2) {
        if (param2->is_symbol()) {
            t.param2 = param2->as_symbol().symbol;
        } else if (param2->is_value()) {
            t.param2 = param2;
        } else {
            build(param2, sym);
            t.param2 = current->triples.back();
        }
    }
    current->add(t);
}

ir* ir_builder::new_ir() {
    ir* c = current;
    current = new ir{};
    c->next = current;
    current->add({ir_op::NOOP});
    return current;
}
