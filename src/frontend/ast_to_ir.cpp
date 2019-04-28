#include "frontend/ast_to_ir.h"

#include "common/ast.h"
#include "common/symbol_table.h"
#include "common/utils.h"
#include "common/type.h"

ir_builder::ir_builder::ir_builder(parse_info& p) : p(p) {
    ir_triple* start = new ir_triple{ir_op::NOOP};
    triples.push_back(start);
    blocks.emplace(ir_triple_range{start, start});
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
        case east_type::CLOSURE: [[fallthrough]]; // Put the code somewhere!
        case east_type::BLOCK: break;
        case east_type::NONE: return;
        default: 
            if (node->is_value()) {
                add(VALUE, sym, node);
            } else if (node->is_symbol()) {
                add(SYMBOL, sym, node);
            } else {
                logger::error() << "Found wrong ast_type at start: " << node->t << logger::nend;
            }
            return;
    }
    
    if (node->is_block()) {
        start_block();
        
        for (auto bnode : node->as_block().stmts) {
            build(bnode, node->as_block().st);
        }
        
        end_block();
    } else if (node->is_binary()) {
        using Grammar::Symbol;
        auto& bin = node->as_binary();
        switch (bin.op) {
            case Symbol::SYMDECL: { // Function, or type
                if (bin.left->is_symbol()) {
                    switch (bin.left->as_symbol().symbol->t) {
                        case est_entry_type::OVERLOAD: {
                            auto dir = labeled.find(bin.left->as_symbol().symbol);
                            if (bin.right->is_none()) {
                                if (dir == labeled.end()) {
                                    ir_triple* noop = new ir_triple{NOOP};
                                    triples.push_back(noop);
                                    labeled[bin.left->as_symbol().symbol] = noop;
                                }
                            } else {
                                ir_triple* prev_cur = add(JUMP);
                                
                                if (dir == labeled.end()) {
                                    labeled[bin.left->as_symbol().symbol] = add(NOOP);
                                } else {
                                    current()->next = dir->second;
                                    current_block().add(dir->second);
                                }
                                
                                build(bin.right, bin.right->as_block().st);
                                add(NOOP);
                                
                                prev_cur->param1 = prev_cur->cond = current();
                            }
                        }
                        case est_entry_type::TYPE: 
                            return;
                        default:
                            logger::error() << "Wrong declaration type: " << bin.left->as_symbol().symbol->t << logger::nend;
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
                        
                        add(COPY, right.st, left.stmts[i], right.stmts[i]);
                    }
                }
                break;
            }
            case Symbol::KWIF: {
                auto& lblk = bin.left->as_block();
                if (lblk.stmts.size() > 1) {
                    start_block();
                    for (u64 i = 0; i < lblk.stmts.size() - 1; ++i) {
                        build(lblk.stmts[i], lblk.st);
                    }
                }
                build(lblk.stmts.back(), lblk.st);
                if (bin.right->is_block()) {
                    ir_triple* if_jmp = add(IF_ZERO, current());
                    build(bin.right, sym);
                    add(NOOP);
                    if_jmp->param2 = if_jmp->cond = current();
                } else if(bin.right->is_binary() && bin.right->as_binary().op == Symbol::KWELSE) {
                    auto& elsebin = bin.right->as_binary();
                    
                    ir_triple* if_jmp = add(IF_ZERO, current());
                    
                    build(elsebin.left, sym);
                    
                    ir_triple* else_jmp = add(JUMP);
                    add(NOOP);
                    if_jmp->param2 = if_jmp->cond = current();
                    
                    build(elsebin.right, sym);
                    add(NOOP);
                    
                    else_jmp->param1 = else_jmp->cond = current();
                } else {
                    logger::error() << "Unexpected if continuation " << bin.right->t << logger::nend;
                }
                
                break;
            }
            case Symbol::KWFOR: {
                auto& forcond = bin.left->as_unary();
                start_block();
                current_block().add_end(create(NOOP));
                
                ir_triple* setup_end{nullptr};
                ir_triple* step{nullptr};
                ir_triple* condition{nullptr};
                
                switch (forcond.op) {
                    case Symbol::KWFORCLASSIC: {
                        auto& decl = forcond.node->as_block();
                        build(decl.stmts[0], decl.st);
                        setup_end = current();
                        
                        step = add(NOOP);
                        for (u64 i = 2; i < decl.stmts.size(); ++i) {
                            build(decl.stmts[i], decl.st);
                        }
                        
                        condition = add(NOOP);
                        build(decl.stmts[1], decl.st);
                        add(IF_ZERO, current(), current_end());
                        current()->cond = current_end();
                        
                        break;
                    }
                    case Symbol::KWFOREACH: {
                        auto& decl = forcond.node->as_binary();
                        add(LENGTH, sym, decl.right);
                        ir_triple* len = current();
                        add(TEMP, (u64) 0);
                        ir_triple* tmp = current();
                        setup_end = tmp;
                        
                        step = add(ADD, tmp, 1);
                        add(COPY, tmp, current());
                        
                        condition = add(LESS, tmp, len);
                        add(IF_ZERO, current(), current_end());
                        current()->cond = current_end();
                        
                        break;
                    }
                    case Symbol::KWFORLUA: {
                        auto& decl = forcond.node->as_binary();
                        auto& vals = decl.right->as_block();
                        add(COPY, decl.left->as_block().stmts[0], vals.stmts[0]);
                        build(vals.stmts[1], sym);
                        ir_triple* last_val = current();
                        
                        if (vals.stmts.size() > 2) {
                            build(vals.stmts[2], sym);
                        } else {
                            add(VALUE, 1);
                        }
                        
                        ir_triple* add_step = current();
                        setup_end = add_step;
                        
                        step = add(ADD, decl.left->as_block().stmts[0], add_step);
                        add(COPY, decl.left->as_block().stmts[0], current());
                        condition = add(LESS, add_step, (u64) 0);
                        ir_triple* neg_step_jmp = add(IF_ZERO);
                        add(LESS_EQUALS, decl.left->as_block().stmts[0], last_val);
                        
                        add(IF_ZERO, current(), current_end());
                        current()->cond = current_end();
                        
                        ir_triple* less_eq_z_jmp  = add(JUMP);
                        add(GREATER_EQUALS, step, last_val);
                        neg_step_jmp->param1 = neg_step_jmp->cond = current();
                        
                        add(IF_ZERO, current(), current_end());
                        current()->cond = current_end();
                        
                        add(NOOP);
                        less_eq_z_jmp->param1 = less_eq_z_jmp->cond = current();
                        
                        break;
                    }
                    default:
                        logger::error() << "Unexpected for type " << forcond.op << logger::nend;
                        break;
                }
                
                ir_triple* end_jmp = create(JUMP, step);
                end_jmp->cond = step;
                current_block().add_end(end_jmp);
                
                ir_triple* skip_step = create(JUMP, condition);
                skip_step->cond = condition;
                setup_end->next = skip_step;
                skip_step->next = step;
                
                build(bin.right, sym);
                end_block();
                break;
            }
            case Symbol::MULTIPLY: [[fallthrough]];
            case Symbol::GREATER: [[fallthrough]];
            case Symbol::LESS: [[fallthrough]];
            case Symbol::ADD: [[fallthrough]];
            case Symbol::SUBTRACT: [[fallthrough]];
            case Symbol::POWER: [[fallthrough]];
            case Symbol::DIVIDE: [[fallthrough]];
            case Symbol::MODULO: [[fallthrough]];
            case Symbol::AND: [[fallthrough]];
            case Symbol::OR: [[fallthrough]];
            case Symbol::XOR: [[fallthrough]];
            case Symbol::SHIFT_LEFT: [[fallthrough]];
            case Symbol::SHIFT_RIGHT: [[fallthrough]];
            case Symbol::ROTATE_LEFT: [[fallthrough]];
            case Symbol::ROTATE_RIGHT: [[fallthrough]];
            case Symbol::EQUALS: [[fallthrough]];
            case Symbol::NOT_EQUALS: [[fallthrough]];
            case Symbol::GREATER_OR_EQUALS: [[fallthrough]];
            case Symbol::LESS_OR_EQUALS: {
                ir_op::code op = symbol_to_ir_code(bin.op);
                add(op, sym, bin.left, bin.right);
                break;
            }
            case Symbol::ADD_ASSIGN: [[fallthrough]];
            case Symbol::SUBTRACT_ASSIGN: [[fallthrough]];
            case Symbol::MULTIPLY_ASSIGN: [[fallthrough]];
            case Symbol::POWER_ASSIGN: [[fallthrough]];
            case Symbol::DIVIDE_ASSIGN: [[fallthrough]];
            case Symbol::AND_ASSIGN: [[fallthrough]];
            case Symbol::OR_ASSIGN: [[fallthrough]];
            case Symbol::XOR_ASSIGN: [[fallthrough]];
            case Symbol::SHIFT_LEFT_ASSIGN: [[fallthrough]];
            case Symbol::SHIFT_RIGHT_ASSIGN: [[fallthrough]];
            case Symbol::ROTATE_LEFT_ASSIGN: [[fallthrough]];
            case Symbol::ROTATE_RIGHT_ASSIGN: [[fallthrough]];
            case Symbol::CONCATENATE_ASSIGN: [[fallthrough]];
            case Symbol::BIT_SET_ASSIGN: [[fallthrough]];
            case Symbol::BIT_CLEAR_ASSIGN: [[fallthrough]];
            case Symbol::BIT_TOGGLE_ASSIGN: [[fallthrough]];
            case Symbol::ASSIGN: {
                ir_op::code op = symbol_to_ir_code(Grammar::without_assign(bin.op));
                auto& lblk = bin.left->as_block();
                auto& rblk = bin.right->as_block();
                u64 needed = lblk.stmts.size();
                std::vector<ir_triple*> vals{};
                for (u64 i = 0; i < rblk.stmts.size() && vals.size() < needed; ++i) {
                    build(rblk.stmts[i], rblk.st);
                    ir_triple* val = current();
                    if (rblk.stmts[i]->get_type()->is_combination()) {
                        auto& cmb = rblk.stmts[i]->get_type()->as_combination().types;
                        u64 j = 0;
                        while (vals.size() < needed && j < cmb.size()) {
                            vals.push_back(add(OFFSET, val, j++));
                        }
                    } else {
                        vals.push_back(val);
                    }
                }
                for (u64 i = 0; i < lblk.stmts.size(); ++i) {
                    build(lblk.stmts[i]);
                    ir_triple* store = current();
                    if (op != NOOP) {
                        add(op, store, vals[i]);
                    }
                    
                    add(COPY, store, current());
                }
                break;
            }
            default:
                logger::error() << "Unhandled binary case " << bin.op << logger::nend;
                break;
        }
    } else if (node->is_unary()) {
        using Grammar::Symbol;
        auto& un = node->as_unary();
        if (un.post) {
            switch (un.op) {
                case Symbol::KWLABEL: {
                    labeled[un.node->as_symbol().symbol] = add(NOOP); // GOTO Handled later
                    break;
                }
                case Symbol::KWRETURN: {
                    u64 rets = 0;
                    if (un.node->is_block()) {
                        auto& blk = un.node->as_block();
                        rets = blk.stmts.size();
                        for (int i = 0; i < blk.stmts.size(); ++i) {
                            add(RETVAL, blk.st, blk.stmts[i]);
                        }
                    }
                    // TODO Proper destructors
                    // TODO Return sigs as 0
                    add(RETURN, rets);
                    break;
                }
                case Symbol::KWRAISE: {
                    add(RETVAL, sym, un.node);
                    add(RETURN, 1);
                    // TODO Return other values as 0
                    break;
                }
                default: 
                    logger::error() << "Unhandled unary case " << un.op << logger::nend;
            }
        } else {
            // Increment, decrement, add, subtract, length, not, lnot, at, address
            switch (un.op) {
                case Symbol::INCREMENT:
                    add(INCREMENT, sym, un.node);
                    break;
                case Symbol::DECREMENT:
                    add(DECREMENT, sym, un.node);
                    break;
                case Symbol::ADD:
                    add(AND, sym, un.node, un.node);
                    break;
                case Symbol::SUBTRACT:
                    add(NEGATE, sym, un.node);
                    break;
                case Symbol::LENGTH:
                    add(LENGTH, sym, un.node);
                    break;
                case Symbol::NOT:
                    add(NOT, sym, un.node);
                    break;
                case Symbol::LNOT: {
                    ir_triple* lnot = add(OR, sym, un.node);
                    lnot->param2 = ~1ull;
                    add(NOT, lnot);
                    break;
                }
                case Symbol::AT:
                    add(DEREFERENCE, sym, un.node);
                    break;
                case Symbol::ADDRESS:
                    add(ADDRESS, sym, un.node);
                    break;
                default:
                    logger::error() << "Wrong symbol type: " << un.op << logger::nend;
            }
        }
    } else if (node->is_closure()) {
        
    } else {
        logger::error() << "Wrong node type " << node->t << logger::nend;
    }
}

void ir_builder::optimize() {
    return;
}

ir_triple* ir_builder::get() {
    return triples.front();
}

ir_triple* ir_builder::add(ir_triple t) {
    ir_triple* tp = new ir_triple{t};
    triples.push_back(tp);
    current_block().add(tp);
    return tp;
}

ir_triple* ir_builder::add(ir_triple* tp) {
    current_block().add(tp);
    return tp;
}

ir_triple* ir_builder::add(ir_op::code code, symbol_table* sym, ast* param1, ast* param2) {
    return add(create(code, sym, param1, param2));
}

ir_triple* ir_builder::add(ir_op::code code) {
    return add(create(code));
}

ir_triple* ir_builder::add(ir_op::code code, ir_triple::ir_triple_param param1, ir_triple::ir_triple_param param2) {
    return add(create(code, param1, param2));
}

ir_triple* ir_builder::create(ir_op::code code, symbol_table* sym, ast* param1, ast* param2) {
    ir_triple* tp = new ir_triple{code};
    triples.push_back(tp);
    ir_triple& t = *tp;
    
    if (param1->is_symbol()) {
        t.param1 = param1->as_symbol().symbol;
    } else if (param1->is_value()) {
        t.param1 = param1;
    } else {
        build(param1, sym);
        t.param1 = current();
    }
    if (param2) {
        if (param2->is_symbol()) {
            t.param2 = param2->as_symbol().symbol;
        } else if (param2->is_value()) {
            t.param2 = param2;
        } else {
            build(param2, sym);
            t.param2 = current();
        }
    }
    return tp;
}

ir_triple* ir_builder::create(ir_op::code code) {
    ir_triple* tp = new ir_triple{code};
    triples.push_back(tp);
    return tp;
}

ir_triple* ir_builder::create(ir_op::code code, ir_triple::ir_triple_param param1, ir_triple::ir_triple_param param2) {
    ir_triple* tp = new ir_triple{code, param1, param2};
    triples.push_back(tp);
    return tp;
}

void ir_builder::start_block() {
    ir_triple* noop = new ir_triple{ir_op::NOOP};
    current()->next = noop;
    blocks.emplace(ir_triple_range{noop, noop});
}

void ir_builder::end_block() {
    auto& prev = blocks.top();
    prev.finish();
    blocks.pop();
    blocks.top().add(prev.start);
}

ir_triple* ir_builder::current() {
    return blocks.top().start.end;
}

ir_triple* ir_builder::current_end() {
    return blocks.top().end.start ? blocks.top().end.start : blocks.top().start.end;
}

block& ir_builder::current_block() {
    return blocks.top();
}

ir_op::code ir_builder::symbol_to_ir_code(Grammar::Symbol sym) {
    using namespace ir_op;
    using namespace Grammar;
    
    switch (sym) {
        case Symbol::MULTIPLY: return MULTIPLY;
        case Symbol::GREATER: return GREATER;
        case Symbol::LESS: return LESS;
        case Symbol::ADD: return ADD;
        case Symbol::SUBTRACT: return SUBTRACT;
        case Symbol::POWER: return POWER;
        case Symbol::DIVIDE: return DIVIDE;
        case Symbol::MODULO: return MODULO;
        case Symbol::AND: return AND;
        case Symbol::OR: return OR;
        case Symbol::XOR: return XOR;
        case Symbol::SHIFT_LEFT: return SHIFT_LEFT;
        case Symbol::SHIFT_RIGHT: return SHIFT_RIGHT;
        case Symbol::ROTATE_LEFT: return ROTATE_LEFT;
        case Symbol::ROTATE_RIGHT: return ROTATE_RIGHT;
        case Symbol::EQUALS: return EQUALS;
        case Symbol::NOT_EQUALS: return NOT_EQUALS;
        case Symbol::GREATER_OR_EQUALS: return GREATER_EQUALS;
        case Symbol::LESS_OR_EQUALS: return LESS_EQUALS;
        default: return NOOP;
    }
}

