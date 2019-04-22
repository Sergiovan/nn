#include "frontend/ast_to_ir.h"

#include "common/ast.h"
#include "common/symbol_table.h"
#include "common/utils.h"

ir_builder::ir_builder::ir_builder(parse_info& p) : p(p) {
    current = base = new ir{};
    irs.push_back(base);
    blocks.emplace(base);
}

void ir_builder::build(ast* node, symbol_table* sym) {    
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
    
    if (node->t == east_type::BLOCK) {
        blocks.emplace(new_ir());
        
        for (auto bnode : node->as_block().stmts) {
            build(bnode, node->as_block().st);
        }
        
        blocks.pop();
    } else if (node->t == east_type::BINARY) {
        using Grammar::Symbol;
        auto& bin = node->as_binary();
        switch (bin.op) {
            case Symbol::SYMDECL: {
                // TODO
            }
            default:
                break;
        }
    }
}

void ir_builder::optimize() {
    return;
}

void ir_builder::add_proper(ir_triple& triple, ast* node, symbol_table* st) {
    if (node->is_symbol()) {
        triple.param2 = node->as_symbol().symbol;
    } else if (node->is_value()) {
        triple.param2 = node;
    } else {
        build(node, st);
        triple.param2 = current->triples.back();
    }
    current->add(triple);
}

ir* ir_builder::new_ir() {
    ir* c = current;
    current = new ir{};
    c->next = current;
    return current;
}
