#include "ast_walker.h"

#include "ast.h"

ast_walker::ast_walker(ast* root) : current(root) { }

ast* ast_walker::next() {
    if (current) {
        ast* ret = current;
        switch (ret->t) {
            case east_type::TYPE: {
                auto& sizes = ret->as_nntype().array_sizes;
                for (auto it = sizes.rbegin(); it != sizes.rend(); ++it) {
                    stored.push(*it);
                }
                goto empty_body;
            }
            case east_type::BLOCK: {
                auto& block = ret->as_block().stmts;
                for(auto it = block.rbegin(); it != block.rend(); ++it) {
                    stored.push(*it);
                }
            } 
            empty_body:
            [[fallthrough]];
            case east_type::SYMBOL: [[fallthrough]];
            case east_type::BYTE: [[fallthrough]];
            case east_type::WORD: [[fallthrough]];
            case east_type::DWORD: [[fallthrough]];
            case east_type::QWORD: [[fallthrough]];
            case east_type::STRING: [[fallthrough]];
            case east_type::ARRAY: [[fallthrough]];
            case east_type::NONE: [[fallthrough]];
            case east_type::STRUCT: 
                if (!stored.empty()) {
                    current = stored.top();
                    stored.pop();
                } else {
                    current = nullptr;
                }
                return ret;
            case east_type::CLOSURE: {
                auto& closure = ret->as_closure();
                current = closure.function;
                return ret;
            }
            case east_type::FUNCTION: {
                auto& function = ret->as_function();
                current = function.block;
                return ret;
            }
            case east_type::PRE_UNARY: [[fallthrough]];
            case east_type::POST_UNARY: 
                current = ret->as_unary().node;
                return ret;
            case east_type::BINARY: 
                current = ret->as_binary().left;
                stored.push(ret->as_binary().right);
                return ret;
            default:
                return nullptr;
        }
    } else {
        if (!stored.empty()) {
            while (!(current = stored.top())) {
                stored.pop();
            }
            return next();
        }
        return nullptr;
    }
}
