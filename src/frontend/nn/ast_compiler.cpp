#include "frontend/nn/ast_compiler.h"
#include "common/utils.h"

#include "common/grammar.h"

#include "common/ast.h"
#include "common/symbol_table.h"
#include "common/type_table.h"
#include "common/ir.h"

using namespace ast_compiler_ns;
using namespace std::string_literals;

context context::down() {
    context nw{};
    nw.function_type = function_type;
    nw.function_start = function_start;
    nw.function_end = function_end;
    nw.function_returns = function_returns;
    
    nw.block_start = block_start;
    nw.block_end = block_end;
    
    nw.loop_start = loop_start;
    nw.loop_end = loop_end;
    nw.loop_breaks = loop_breaks;
    nw.loop_continues = loop_continues;
    
    nw.breaks = false;
    nw.continues = false;
    nw.returns = false;
    nw.in_switch = false;
    
    nw.in_try = in_try;
    
    return nw;
}

void context::clear() {
    breaks = continues = returns = false;
}

void context::update(context& o) {
    breaks |= o.breaks;
    continues |= o.continues;
    returns |= o.returns;
}

ast_compiler::ast_compiler(ast* root, symbol_table* st, type_table& tt) : walker(root), root(nullptr), st(st), tt(tt) { }

ast_compiler::~ast_compiler() {
    for (auto triple : triples) {
        if (triple) delete triple;
    }
}

void ast_compiler::compile() {
    c = root = make();
    
    ast* a = walker.next(); // Block
    auto& block = a->as_block();
    context ctx;
    for (u64 i = 0; i < block.stmts.size(); ++i) {
        build(st, ctx);
    }
    
    // Take care of global declarations here
}

ir_triple* ast_compiler::get() {
    return root;
}

ir_triple* ast_compiler::build(symbol_table* st, context& ctx) {
    ast* a = walker.next();
    
    switch(a->t) {
        case east_type::BLOCK: {
            auto& block = a->as_block();
            context nctx = ctx.down();
            (nctx.block_start = next({ir_op::BLOCK_START}))->label = "Block start"s;
            next();
            (nctx.block_end = ahead({ir_op::BLOCK_END}))->label = "Block end"s;
            
            for (u64 i = 0; i < block.stmts.size(); ++i) {
                build(block.st, nctx);
            }
            
            c = nctx.block_end;
            
            // Flow control
            if (nctx.returns) {
                next({ir_op::IF_NOT_ZERO, ctx.function_returns});
                jump_to(ctx.block_end);
            }
            if (nctx.breaks) {
                next({ir_op::IF_NOT_ZERO, ctx.loop_breaks});
                jump_to(ctx.block_end);
            }
            if (nctx.continues && !ctx.in_switch) {
                next({ir_op::IF_NOT_ZERO, ctx.loop_continues});
                jump_to(ctx.block_end);
            }
            
            ctx.update(nctx);
            
            return nctx.block_start;
        }
        case east_type::SYMBOL: 
            return next({ir_op::SYMBOL, a}, a->get_type());
        case east_type::BYTE: [[fallthrough]];
        case east_type::WORD: [[fallthrough]];
        case east_type::DWORD: [[fallthrough]];
        case east_type::QWORD: [[fallthrough]];
        case east_type::STRING: [[fallthrough]];
        case east_type::STRUCT: [[fallthrough]];
        case east_type::ARRAY: 
            return next({ir_op::VALUE, a}, a->get_type());
        case east_type::CLOSURE: {
            ir_triple* clos = next({ir_op::VALUE, nullptr}, a->get_type());
            auto f = build(st, ctx); // Function
            // Here is where we actually create a closure, but let's skip that for now
            clos->op1 = f;
            return clos;
        }
        case east_type::FUNCTION: {
            auto& f = a->as_function();
            
            context nctx{};
            nctx.function_type = f.t;
            ir_triple* buff = c;
            c = make({ir_op::FUNCTION_START}, f.t);
            c->next = root;
            c->label = "Function start"s;
            root = c;
            nctx.block_start = nctx.function_start = c;
            (nctx.function_returns = next({ir_op::TEMP, 0}, type_table::t_bool))->label = "Function returning"s;
            ahead({ir_op::RETURN});
            (nctx.block_end = nctx.function_end = ahead({ir_op::FUNCTION_END}))->label = "Function end"s;
            
            auto& block = walker.next()->as_block(); // We do this manually
            
            for (u64 i = 0; i < block.stmts.size(); ++i) {
                build(block.st, nctx);
            }
            
            c = nctx.function_end;
            
            
            c = buff;
            functions[a] = nctx.function_start;
            
            return nctx.function_start;
        }
        case east_type::PRE_UNARY: [[fallthrough]];
        case east_type::POST_UNARY: 
            return build(a->as_unary(), st, ctx);
        case east_type::BINARY: 
            return build(a->as_binary(), st, ctx);
        case east_type::NONE: [[fallthrough]];
        case east_type::TYPE: 
            break;
        default:
            logger::error() << "AST of type " << a->t << logger::nend;
            return nullptr;
    }
    
    return nullptr;
}

ir_triple* ast_compiler::build(ast_unary& un, symbol_table* st, context& ctx) {
    using Grammar::Symbol;
    using namespace ir_op;
    
    if (un.post) {
        switch (un.op) {
            case Symbol::INCREMENT: {
                ir_triple* elem = build(st, ctx);
                ir_triple* ret = next({VALUE, elem}, elem->res_type);
                next({INCREMENT, elem}, un.t);
                return ret;
            }
            case Symbol::DECREMENT: {
                ir_triple* elem = build(st, ctx);
                ir_triple* ret = next({VALUE, elem}, elem->res_type);
                next({DECREMENT, elem}, un.t);
                return ret;
            }
            default:
                logger::error() << un.op << " not implemented for build(pre_binary)" << logger::nend;
                return nullptr;
        }
    } else {
        switch (un.op) {
            case Symbol::KWDELETE: {
                auto& block = walker.next()->as_block();
                for (auto& stmt : block.stmts) {
                    (void) stmt;
                    ir_triple* elem = build(st, ctx);
                    next({DELETE, elem});
                }
                return c;
            }
            case Symbol::KWLABEL: {
                auto label = labels.find(walker.next());
                if (label == labels.end()) {
                    labels[un.node] = next();
                } else {
                    next(label->second);
                }
                c->label = "label "s + un.node->as_symbol().get_name();
                return c;
            }
            case Symbol::KWRETURN: {
                walker.next();
                if (un.node->is_block()) {
                    u64 i = 0;
                    for (auto& stmt : un.node->as_block().stmts) {
                        (void) stmt;
                        ir_triple* ret = build(st, ctx);
                        next({RETVAL, ret, i}, ctx.function_type->get_function_returns());
                        ++i;
                    }
                    auto ret = ctx.function_type->get_function_returns();
                    if (ret->is_combination()) {
                        auto& rets = ret->as_combination();
                        for (; i < rets.types.size(); ++i) {
                            next({RETVAL, 0, i}, ctx.function_type->get_function_returns());
                        }
                    } else if (!i && !ret->is_primitive(etype_ids::VOID)) {
                        next({RETVAL, 0, 0}, ctx.function_type->get_function_returns());
                    }
                }
                ctx.returns = true;
                next({COPY, 1, ctx.function_returns});
                next({JUMP, ctx.block_end});
                jump_to(ctx.block_end);
                return c;
            }
            case Symbol::KWRAISE: {
                ir_triple* val = build(st, ctx);
                auto ret = ctx.function_type->get_function_returns();
                if (ret->is_combination()) {
                    auto& rets = ret->as_combination();
                    for (u64 i = 0; i < rets.types.size(); ++i) {
                        auto* t = rets.types[i];
                        if (t->is_primitive(etype_ids::SIG)) {
                            next({RETVAL, val, i}, ctx.function_type->get_function_returns());
                        } else {
                            next({RETVAL, 0, i}, ctx.function_type->get_function_returns());
                        }
                    }
                } else {
                    next({RETVAL, val, 0}, ctx.function_type->get_function_returns());
                }
                ctx.returns = true;
                next({COPY, 1, ctx.function_returns});
                next({JUMP, ctx.block_end});
                jump_to(ctx.block_end);
                return c;
            }
            case Symbol::KWDEFER: {
                ir_triple* before_begin = c;
                build(st, ctx);
                ir_triple* end = c;
                next();
                
                move_to(before_begin, end, ctx.block_end);
                return c;
            }
            case Symbol::KWNAMESPACE: {
                build(st, ctx);
                return c;
            }
            case Symbol::KWUSING: 
                walker.next(); 
                return c;
            case Symbol::KWCONTINUE: {
                walker.next();
                if (ctx.loop_end) {
                    ctx.continues = true;
                    if (!ctx.in_switch) {
                        next({COPY, 1, ctx.loop_continues});
                        next({JUMP, ctx.block_end});
                        jump_to(ctx.block_end);
                    }
                } else {
                    logger::error() << "continue called outside of loop";
                }
                return c;
            }
            case Symbol::KWBREAK: {
                walker.next();
                if (ctx.loop_end) {
                    ctx.breaks = true;
                    next({COPY, 1, ctx.loop_breaks});
                    next({JUMP, ctx.block_end});
                    jump_to(ctx.block_end);
                } else {
                    logger::error() << "break called outside of loop";
                }
                return c;
            }
            case Symbol::KWLEAVE: {
                walker.next();
                if (ctx.block_end && ctx.block_end != ctx.function_end) {
                    next({JUMP, ctx.block_end});
                    jump_to(ctx.block_end);
                } else {
                    logger::error() << "leave called outside of leavable block";
                }
                return c;
            }
            case Symbol::KWGOTO: {
                walker.next();
                auto sym = walker.next();
                auto label = labels.find(sym);
                if (label == labels.end()) {
                    ir_triple* lbl = make();
                    labels.insert({sym, lbl});
                    next({JUMP});
                    jump_to(lbl);
                } else {
                    next({JUMP});
                    jump_to(label->second);
                }
                // TODO Breaks?
                return c;
            }
            case Symbol::INCREMENT: {
                ir_triple* elem = build(st, ctx);
                return next({INCREMENT, elem}, un.t);
            }
            case Symbol::DECREMENT: {
                ir_triple* elem = build(st, ctx);
                return next({DECREMENT, elem}, un.t);
            }
            case Symbol::ADD: { // Does nothing?
                build(st, ctx);
                return c;
            }
            case Symbol::SUBTRACT: {
                ir_triple* elem = build(st, ctx);
                return next({NEGATE, elem}, un.t);
            }
            case Symbol::LENGTH: {
                ir_triple* elem = build(st, ctx);
                return next({LENGTH, elem}, un.t);
            }
            case Symbol::NOT: {
                ir_triple* elem = build(st, ctx);
                return next({NOT, elem}, un.t);
            }
            case Symbol::LNOT: {
                ir_triple* elem = build(st, ctx);
                return next({EQUALS, elem, 0}, un.t);
            }
            case Symbol::AT: {
                ir_triple* elem = build(st, ctx);
                return next({DEREFERENCE, elem}, un.t);
            }
            case Symbol::ADDRESS: {
                ir_triple* elem = build(st, ctx);
                return next({ADDRESS, elem}, un.t);
            }
            case Symbol::KWCASE: {
                walker.next();
                build(st, ctx);
                return c;
            }
            case Symbol::KWELSE: {
                walker.next();
                build(st, ctx);
                return c;
            }
            default:
                logger::error() << un.op << " not implemented for build(pre_binary)" << logger::nend;
                return nullptr;
        }
    }
}

ir_triple* ast_compiler::build(ast_binary& bin, symbol_table* st, context& ctx) {
    using Grammar::Symbol;
    using namespace ir_op;
    
    switch(bin.op) {
        case Symbol::SYMDECL: {
            if (bin.left->is_symbol()) { // Type or function
                auto& sym = walker.next()->as_symbol();
                switch (sym.symbol->t) {
                    case est_entry_type::OVERLOAD: {
                        if (bin.right->is_none()) {
                            walker.next(); // Ignore this
                            return c;
                        }
                        
                        auto f = build(st, ctx); // Function
                        labels[bin.left] = f;
                        return f;
                    }
                    case est_entry_type::TYPE: {
                        if (bin.right->is_none()) { // Ignore
                            walker.next();
                            return c;
                        }
                        
                        next();
                        auto before = c;
                        before->label = "Before struct"s;
                        
                        auto& block = walker.next()->as_block();
                        for (u64 i = 0; i < block.stmts.size(); ++i) {
                            if (block.stmts[i]->get_type()->is_function(false)) { // Only functions for now
                                build(block.st, ctx);
                            } else {
                                build(block.st, ctx); // TODO Constructors destructors?
                            }
                        }
                        next();
                        auto after = c;
                        after->label = "After struct"s;
                        
                        before->next = after; // Oops
                        
                        return c;
                    }
                    case est_entry_type::FIELD: { // Ignore these, but not really TODO All
                        if (bin.left->is_block()) {
                            auto& left = walker.next()->as_block();
                            for (u64 i = 0; i < left.stmts.size(); ++i) {
                                auto stmt = walker.next();
                                (void)stmt;
                                // DESTRUCTORS GO HERE
                            }
                            if (bin.right->is_none()) {
                                walker.next(); // Ignore
                                return c; 
                            }
                            auto& right = walker.next()->as_block();
                            u64 l = 0;
                            for (u64 r = 0; r < right.stmts.size() && l < left.stmts.size(); ++r, ++l) {
                                auto elem = build(st, ctx);
                                if (elem->res_type->is_combination()) {
                                    auto& comb = elem->res_type->as_combination().types;
                                    u64 combn = 0;
                                    while (l < left.stmts.size() && combn < comb.size()) { // Initialize from combination
                                        next({INDEX, elem, combn}, comb[combn]);
                                        next({COPY, c, left.stmts[l]});
                                        ++l;
                                        ++combn;
                                    }
                                } else {
                                    next({COPY, elem, left.stmts[l]}); // Initialize one element
                                }
                            }
                            while (l < left.stmts.size()) { // Zero initialize
                                // TODO Constructors??
                                next({ZERO, left.stmts[l]});
                                ++l;
                            }
                            return c; // Done
                        } else {
                            if (bin.right->is_none()) {
                                walker.next(); // Ignore
                                return c;
                            }
                            auto right = build(st, ctx);
                            next({COPY, right, bin.left});
                            return c;
                        }
                    }
                    default:
                        logger::error() << "Wrong declaration type: " << sym.symbol->t << logger::nend;
                }
                return c;
            } else { // Variables
                auto& left = walker.next()->as_block();
                ir_triple* syms[left.stmts.size()];
                for (u64 i = 0; i < left.stmts.size(); ++i) {
                    syms[i] = build(st, ctx);
                    // DESTRUCTORS GO HERE
                }
                if (bin.right->is_none()) {
                    walker.next(); // Ignore
                    
                    for (auto triple : syms) {
                        next({COPY, 0, triple});
                    }
                    
                    return c; 
                }
                auto& right = walker.next()->as_block();
                u64 l = 0;
                for (u64 r = 0; r < right.stmts.size() && l < left.stmts.size(); ++r, ++l) {
                    auto elem = build(st, ctx);
                    if (elem->res_type->is_combination()) {
                        auto& comb = elem->res_type->as_combination().types;
                        u64 combn = 0;
                        while (l < left.stmts.size() && combn < comb.size()) { // Initialize from combination
                            next({INDEX, elem, combn}, comb[combn]);
                            next({COPY, c, syms[l]});
                            ++l;
                            ++combn;
                        }
                    } else {
                        next({COPY, elem, syms[l]}); // Initialize one element
                    }
                }
                while (l < left.stmts.size()) { // Zero initialize
                    // TODO Constructors??
                    next({ZERO, left.stmts[l]});
                    ++l;
                }
                return c; // Done
            }
        }
        case Symbol::KWIF: {
            auto& ifblock = walker.next()->as_block();
            context nctx = ctx.down();
            (nctx.block_start = next({ir_op::BLOCK_START}))->label = "Block start"s;
            next();
            (nctx.block_end = ahead({ir_op::BLOCK_END}))->label = "Block end"s;
            
            ir_triple* cond {nullptr};
            for (u64 i = 0; i < ifblock.stmts.size(); ++i) {
                cond = build(ifblock.st, nctx);
            }
            
            auto if_cond = cond;
            auto else_block = nctx.block_end;
            
            if (bin.right->is_binary()) {
                (else_block = ahead())->label = "Else block"s;
                walker.next();
            }
            
            next({IF_FALSE, if_cond})->label = "If condition"s;
            jump_to(else_block);
            
            build(ifblock.st, nctx); // If block
            next({JUMP})->label = "End of if block"s;
            jump_to(nctx.block_end);
            
            c = else_block;
            
            if (bin.right->is_binary()) {
                build(ifblock.st, nctx);
            }
            
            // Flow control
            if (nctx.returns) {
                next({ir_op::IF_NOT_ZERO, ctx.function_returns});
                jump_to(ctx.block_end);
            }
            if (nctx.breaks) {
                next({ir_op::IF_NOT_ZERO, ctx.loop_breaks});
                jump_to(ctx.block_end);
            }
            if (nctx.continues) {
                next({ir_op::IF_NOT_ZERO, ctx.loop_continues});
                jump_to(ctx.block_end);
            }
            
            ctx.update(nctx);
            
            c = nctx.block_end;
            
            return nctx.block_start;
        }
        case Symbol::TERNARY_CONDITION: {
            auto val = next({TEMP, 0}, bin.t);
            auto cond = build(st, ctx);
            walker.next(); // Skip TERNARY_CHOICE
            auto end = ahead();
            auto if_false = ahead();
            next({IF_FALSE, cond});
            jump_to(if_false);
            build(st, ctx);
            next({COPY, c, val});
            next({JUMP});
            jump_to(end);
            c = if_false;
            build(st, ctx);
            next({COPY, c, val});
            c = end;
            return val;
        }
        case Symbol::KWFOR: {
            auto& forcond = walker.next()->as_unary();
            context nctx = ctx.down();
            (nctx.block_start = next({ir_op::BLOCK_START}))->label = "For start"s;
            nctx.loop_start = nctx.block_start;
            next();
            (nctx.block_end = ahead({ir_op::BLOCK_END}))->label = "For end"s;
            nctx.loop_end = nctx.block_end;
            
            (nctx.loop_continues = next({TEMP, 0}, type_table::t_bool))->label = "For continues"s;
            (nctx.loop_breaks = next({TEMP, 0}, type_table::t_bool))->label = "For breaks"s;
            
            auto for_start = ahead();
            for_start->label = "For block start"s;
            auto condition = ahead();
            condition->label = "For condition"s;
            auto step = ahead();
            step->label = "For step"s;
            auto setup_end = ahead();
            setup_end->label = "For setup end"s;
            
            switch (forcond.op) {
                case Symbol::KWFORCLASSIC: {
                    auto& decl = walker.next()->as_block();
                    build(st, nctx); // TODO Fix
                    next({JUMP});
                    jump_to(condition); // Skip step first time
                    // Setup end bleeds into into step
                    
                    c = condition;
                    build(st, nctx);
                    next({IF_FALSE, c});
                    jump_to(nctx.loop_end);
                    // Bleeds into loop block
                    
                    c = step;
                    for (u64 i = 2; i < decl.stmts.size(); ++i) {
                        build(st, nctx);
                    }
                    // Bleeds into condition
                    break;
                }
                case Symbol::KWFOREACH: {
                    walker.next();
                    walker.next(); // left is a block with a single element
                    auto left = build(st, nctx);
                    auto right = build(st, nctx);
                    auto limit = next({LENGTH, right}, type_table::t_long); 
                    limit->label = "For-each limit"s;
                    auto count = next({TEMP, 0}, c->res_type); 
                    count->label = "For-each count"s;
                    // Setup end bleeds into into step
                    
                    c = step;
                    next({INDEX, right, count}, bin.left->get_type());
                    next({COPY, c, left});
                    auto step_end = next({INCREMENT, count});
                    // Bleeds into loop block
                    
                    c = condition; 
                    next({LESS, count, limit}, type_table::t_bool);
                    next({IF_FALSE, c});
                    jump_to(nctx.loop_end);
                    // bleeds into step
                    
                    move_to(step_end, c, setup_end); // For-each has condition and step reversed
                    
                    break;
                }
                case Symbol::KWFORLUA: {
                    walker.next();
                    walker.next(); // left is a block with a single element
                    auto left = build(st, nctx);
                    auto& vals = walker.next()->as_block();
                    auto start = build(st, nctx);
                    start->label = "For-lua start"s;
                    auto end = build(st, nctx);
                    end->label = "For-lua end"s;
                    ir_triple* luastep{nullptr};
                    if (vals.stmts.size() > 2) {
                        luastep = build(st, nctx);
                    } else {
                        luastep = next({TEMP, 0}, type_table::t_long);
                        next({GREATER, end, start}, type_table::t_long);
                        next({IF_FALSE, c});
                        auto done = ahead();
                        auto negative = ahead();
                        jump_to(negative);
                        next({COPY, 1, luastep});
                        next({JUMP});
                        jump_to(done);
                        c = negative;
                        next({COPY, -1, luastep});
                        c = done;
                    }
                    luastep->label = "For-lua step"s;
                    next({COPY, start, left});
                    next({JUMP});
                    jump_to(condition); // Skip step first time
                    
                    // Setup end bleeds into step                   
                    c = step;
                    next({ADD, left, step}, type_table::t_long);
                    next({COPY, c, left});
                    // Step bleeds into condition
                    
                    c = condition;
                    auto cont = next({TEMP, 0}, type_table::t_bool);
                    auto comp_end = ahead();
                    auto positive = ahead();
                    next({LESS, step, 0}, type_table::t_bool);
                    next({IF_FALSE, c});
                    jump_to(positive);
                    next({GREATER_EQUALS, left, end}, type_table::t_bool);
                    next({COPY, c, cont});
                    next({JUMP});
                    jump_to(comp_end);
                    c = positive;
                    next({LESS_EQUALS, left, end}, type_table::t_bool);
                    next({COPY, c, cont});
                    c = comp_end;
                    next({IF_FALSE, cont});
                    jump_to(nctx.loop_end);
                    
                    // Condition bleeds into loop block
                    break;
                }
                default:
                    logger::error() << "Unexpected for type " << forcond.op << logger::nend;
            }
            
            c = for_start;
            build(st, nctx);
            
            if (nctx.breaks) {
                next({IF_TRUE, nctx.loop_breaks});
                jump_to(nctx.block_end);
            }
            if (nctx.continues) {
                next({IF_FALSE, nctx.loop_continues});
                jump_to(setup_end);
                next({COPY, 0, nctx.loop_continues});
            }
            
            next({JUMP});
            jump_to(setup_end);
            
            c = nctx.block_end;
            
            // Flow control
            if (nctx.returns) {
                next({ir_op::IF_NOT_ZERO, ctx.function_returns});
                jump_to(ctx.block_end);
                ctx.returns = true;
            }
            
            return nctx.block_start;
        }
        case Symbol::KWWHILE: {
            auto& lblock = walker.next()->as_block();
            context nctx = ctx.down();
            (nctx.block_start = next({ir_op::BLOCK_START}))->label = "While start"s;
            nctx.loop_start = nctx.block_start;
            next();
            (nctx.block_end = ahead({ir_op::BLOCK_END}))->label = "While end"s;
            nctx.loop_end = nctx.block_end;
            
            (nctx.loop_continues = next({TEMP, 0}, type_table::t_bool))->label = "While continues"s;
            (nctx.loop_breaks = next({TEMP, 0}, type_table::t_bool))->label = "While breaks"s;
            
            for (u64 i = 0; i < lblock.stmts.size() - 1; ++i) {
                build(lblock.st, nctx);
            }
            
            auto cond_start = next();
            
            auto cond = build(lblock.st, nctx);
            
            next({IF_FALSE, cond});
            jump_to(nctx.block_end);
            
            build(st, nctx);
            
            if (nctx.breaks) {
                next({IF_TRUE, nctx.loop_breaks});
                jump_to(nctx.block_end);
            }
            if (nctx.continues) {
                next({IF_FALSE, nctx.loop_continues});
                jump_to(cond_start);
                next({COPY, 0, nctx.loop_continues});
            }
            
            next({JUMP});
            jump_to(cond_start);
            
            c = nctx.block_end;
            
            // Flow control
            if (nctx.returns) {
                next({ir_op::IF_NOT_ZERO, ctx.function_returns});
                jump_to(ctx.block_end);
                ctx.returns = true;
            }
            
            return nctx.block_start;
        }
        case Symbol::KWCATCH: [[fallthrough]];
        case Symbol::KWSWITCH: {
            context nctx = ctx.down();
            (nctx.block_start = next({ir_op::BLOCK_START}))->label = "Switch start"s;
            nctx.loop_start = nctx.block_start;
            next();
            (nctx.block_end = ahead({ir_op::BLOCK_END}))->label = "Switch end"s;
            nctx.loop_end = nctx.block_end;
            
            // (nctx.loop_continues = next({TEMP, 0}, type_table::t_bool))->label = "Switch continues"s;
            (nctx.loop_breaks = next({TEMP, 0}, type_table::t_bool))->label = "Switch breaks"s;
            
            nctx.in_switch = true;
            
            if (bin.op == Symbol::KWSWITCH) {
                auto& lblock = walker.next()->as_block();
                for (u64 i = 0; i < lblock.stmts.size() - 1; ++i) {
                    build(lblock.st, nctx);
                }
            }
            build(st, nctx);
            
            auto comp_val = c;
            
            auto& rblock = walker.next()->as_block();
            
            std::pair<ir_triple*, ir_triple*> cases[rblock.stmts.size()];
            ir_triple* else_case{nullptr};
            u64 last_normal{0};
            
            for (s64 i = rblock.stmts.size() - 1; i >= 0; --i) {
                auto blok = ahead();
                auto comp = ahead();
                cases[i] = {comp, blok};
                if (rblock.stmts[i]->as_binary().op == Symbol::KWELSE) {
                    else_case = blok;
                } else if (!last_normal) {
                    last_normal = i;
                }
            }
            
            for (u64 i = 0; i < rblock.stmts.size(); ++i) {
                auto& nncase = walker.next()->as_binary();
                c = cases[i].first;
                switch (nncase.op) {
                    case Symbol::KWCASE: {
                        auto& vals = walker.next()->as_block();
                        auto this_case = cases[i].second;
                        for (u64 i = 0; i < vals.stmts.size(); ++i) {
                            build(rblock.st, nctx);
                            next({EQUALS, comp_val, c}, type_table::t_bool);
                            next({IF_TRUE, c});
                            jump_to(this_case);
                        }
                        next({JUMP});
                        if (i == last_normal) {
                            if (else_case) {
                                jump_to(else_case); // Else always at the end
                            } else {
                                jump_to(nctx.block_end); // No else? End switch
                            }
                        } else {
                            if (cases[i+1].second == else_case) {
                                jump_to(cases[i+2].first); // Skip else
                            } else {
                                jump_to(cases[i+1].first); // Go to next 
                            }
                        }
                        c = this_case;
                        build(rblock.st, nctx);
                        break;
                    }
                    case Symbol::KWELSE: {
                        walker.next(); // Ignore left side
                        c = cases[i].second;
                        walker.next(); // Skip unary
                        build(rblock.st, nctx);
                        break;
                    }
                    default:
                        logger::error() << "Wrong keyword inside switch-case: " << nncase.op << logger::nend;
                }
                if (nctx.continues) {
                    next({JUMP});
                    if (i + 1 >= rblock.stmts.size()) {
                        jump_to(nctx.loop_end);
                    } else {
                        jump_to(cases[i+1].second);
                    }
                    nctx.continues = false;
                } else {
                    next({JUMP});
                    jump_to(nctx.loop_end);
                }
            }
            
            c = nctx.block_end;
            
            // Flow control
            if (nctx.returns) {
                next({ir_op::IF_NOT_ZERO, ctx.function_returns});
                jump_to(ctx.block_end);
                ctx.returns = true;
            }
            if (nctx.breaks) {
                next({ir_op::IF_NOT_ZERO, ctx.loop_breaks});
                jump_to(ctx.block_end);
                ctx.breaks = true;
            }
            
            return nctx.block_start;
        }
        case Symbol::KWTRY: { // TODO This doesn't work
            auto& try_block = walker.next()->as_block();
            
            auto outside_try = ahead();
            auto catch_ir = ahead();
            
            context nctx = ctx.down();
            
            nctx.in_try = true;
            
            auto sig_val = next({TEMP, 0}, type_table::t_sig);
            nctx.try_value = sig_val;
            nctx.try_catch = catch_ir;
            
            for (u64 i = 0; i < try_block.stmts.size(); ++i) {
                build(try_block.st, nctx);
            }
            
            next({JUMP});
            jump_to(outside_try);
            
            c = catch_ir;
            if (bin.right->is_unary()) { // raise
                walker.next(); // Ignore
                auto& rets = ctx.function_type->get_function_returns()->as_combination();
                for (u64 i = 0; i < rets.types.size(); ++i) {
                    auto* t = rets.types[i];
                    if (t->is_primitive(etype_ids::SIG)) {
                        next({RETVAL, sig_val, i}, ctx.function_type->get_function_returns());
                    } else {
                        next({RETVAL, 0, i}, ctx.function_type->get_function_returns());
                    }
                }
                ctx.returns = true;
                next({COPY, 1, ctx.function_returns});
                next({JUMP, ctx.block_end});
                jump_to(ctx.block_end);
            } else {
                next({COPY, sig_val, bin.right->as_binary().left->as_symbol().symbol}); // TODO ????
                build(st, ctx); // Should be like a friggin' switch
            }
            
            ctx.update(nctx);
            c = outside_try;
            
            return c;
        }
        case Symbol::FUN_CALL: {
            auto left = build(st, ctx);
            auto& params = bin.right->as_block().stmts;
            ir_triple* ret{nullptr};
            walker.next(); // Skip block
            u64 size = 0;
            bool spread = false;
            if (left->res_type->is_function(false)) {
                auto& rparams = left->res_type->as_function().params;
                spread = rparams.size() && rparams[rparams.size() - 1].flags & eparam_flags::SPREAD;
            } else {
                auto& rparams = left->res_type->as_pfunction().params;
                spread = rparams.size() && rparams[rparams.size() - 1].flags & eparam_flags::SPREAD;
            }
            for (u64 i = 0; i < params.size(); ++i) {
                auto param = build(st, ctx);
                if (param->res_type->is_combination()) {
                    auto& comb = param->res_type->as_combination().types;
                    u64 paramn = 0;
                    for (; paramn < comb.size() && i < params.size(); ++paramn, ++i) {
                        next({INDEX, param, paramn}, comb[paramn]);
                        next({PARAM, c});
                        size += comb[paramn]->get_size();
                    }
                    if (spread) {
                        for (; paramn < comb.size(); ++paramn) {
                            next({INDEX, param, paramn}, comb[paramn]);
                            next({PARAM, c});
                            size += comb[paramn]->get_size();
                        }
                    }
                } else {
                    next({PARAM, param});
                    size += param->res_type->get_size();
                }
            }
            if (bin.left->is_closure() || bin.left->get_type()->is_function(true)) {
                ret = next({CALL_CLOSURE, left, size}, bin.t);
            } else {
                auto& ol = bin.left->as_symbol().symbol->as_overload();
                if (ol.ol->builtin) {
                    ret = next({CALL_BTIN, ol.ol->builtin, size}, bin.t);
                } else {
                    ret = next({CALL, left, size}, bin.t);
                }
            }
            
            if (ctx.in_try) {
                if (ret->res_type->is_combination() || ret->res_type->is_primitive(etype_ids::SIG)) {
                    if (ret->res_type->is_combination()) {
                        auto& ts = ret->res_type->as_combination().types;
                        for (u64 i = 0; i < ts.size(); ++i) {
                            if (ts[i]->is_primitive(etype_ids::SIG)) {
                                next({INDEX, ret, i}, ts[i]);
                                next({COPY, c, ctx.try_value});
                                next({IF_NOT_ZERO, ctx.try_value});
                                jump_to(ctx.try_catch);
                                break;
                            }
                        }
                    } else {
                        next({COPY, ret, ctx.try_value});
                        next({IF_NOT_ZERO, ctx.try_value});
                        jump_to(ctx.try_catch);
                    }
                }
            }
            
            return ret;
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
            auto left = build(st, ctx);
            auto right = build(st, ctx);
            return next({grammar_symbol_to_ir_op(bin.op), left, right}, bin.t);
        }
        case Symbol::CONCATENATE: {
            auto left = build(st, ctx);
            auto right = build(st, ctx);
            return next({CONCATENATE, left, right}, bin.t); // TODO Expand
        }
        case Symbol::INDEX: {
            auto left = build(st, ctx);
            auto right = build(st, ctx);
            if (bin.left->get_type()->is_pointer() && !bin.left->get_type()->is_pointer(eptr_type::ARRAY)) {
                next({MULTIPLY, right, bin.left->get_type()->as_pointer().t->get_size()});
                return next({OFFSET, left, c}, bin.t);
            } else {
                return next({INDEX, left, right}, bin.t);
            }
        }
        case Symbol::KWNEW: {
            auto val = next({TEMP, 0}, bin.t);
            auto& types = walker.next()->as_block();
            
            u64 offset{0};
            
            auto size_tmp = next({TEMP, 0}, type_table::t_long);
            
            for (u64 i = 0; i < types.stmts.size(); ++i) {
                auto& nntype = walker.next()->as_nntype();
                auto loc = next({OFFSET, val, offset}, nntype.t);
                next({COPY, nntype.t->get_size(), size_tmp});
                for (u64 i = 0; i < nntype.array_sizes.size(); ++i) {
                    build(st, ctx);
                    next({MULTIPLY, c, size_tmp}, type_table::t_long);
                    next({COPY, c, size_tmp});
                }
                next({NEW, size_tmp}, bin.t->is_combination() ? bin.t->as_combination().types[i] : bin.t);
                next({COPY, c, loc});
                offset += 8;
            }
            
            offset = 0;
            auto& values = walker.next()->as_block();
            for (u64 i = 0; i < values.stmts.size(); ++i) {
                auto value = build(st, ctx);
                auto& nntype = types.stmts[i]->as_nntype();
                next({OFFSET, val, offset}, nntype.t);
                next({DEREFERENCE, c}, value->res_type);
                next({COPY, value, c});
                offset += 8;
            }
            
            return val;
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
            ir_op::code op = grammar_symbol_to_ir_op(bin.op);
            auto& left_blk = walker.next()->as_block();
            
            ir_triple* lhs[left_blk.stmts.size()];
            
            for (u64 i = 0; i < left_blk.stmts.size(); ++i) {
                lhs[i] = build(st, ctx);
            }
            
            auto& right_blk = walker.next()->as_block();
            u64 l = 0;
            for (u64 r = 0; r < right_blk.stmts.size() && l < left_blk.stmts.size(); ++r, ++l) {
                auto val = build(st, ctx);
                if (val->res_type->is_combination()) {
                    auto& comb = val->res_type->as_combination().types;
                    for (u64 combn = 0; combn < comb.size() && l < left_blk.stmts.size(); ++combn, ++l) {
                        next({INDEX, val, combn}, comb[combn]);
                        if (op != NOOP) {
                            next({op, lhs[l], c});
                        } 
                        next({COPY, c, lhs[l]});
                    }
                } else {
                    next({VALUE, val}, val->res_type);
                    if (op != NOOP) {
                        next({op, lhs[l], val});
                    } 
                    next({COPY, c, lhs[l]});
                }
            }
            
            for (; l < left_blk.stmts.size(); ++l) { // TODO Needed?
                next({ZERO, lhs[l]}, lhs[l]->res_type);
            }
            
            return c;
        }
        case Symbol::CAST: {
            auto left = build(st, ctx); // Left
            walker.next(); // ignore right, type
            ir_op::code op = conversion_operator(left->res_type, bin.right->get_type());
            if (op != NOOP) {
                return next({op, left}, bin.right->get_type());
            } else {
                return next({VALUE, left}, bin.right->get_type());
            }
        }
        case Symbol::LAND: {
            auto temp_val = next({TEMP, 0}, type_table::t_bool);
            auto and_end = ahead();
            build(st, ctx);
            next({COPY, c, temp_val});
            next({IF_FALSE, temp_val});
            jump_to(and_end);
            build(st, ctx);
            next({COPY, c, temp_val});
            c = and_end;
            return temp_val;
        }
        case Symbol::LOR: {
            auto temp_val = next({TEMP, 0}, type_table::t_bool);
            auto or_end = ahead();
            build(st, ctx);
            next({COPY, c, temp_val});
            next({IF_TRUE, temp_val});
            jump_to(or_end);
            build(st, ctx);
            next({COPY, c, temp_val});
            c = or_end;
            return temp_val;
        }
        case Symbol::LXOR: {
            auto left = build(st, ctx);
            auto right = build(st, ctx);
            return next({XOR, left, right}, type_table::t_bool);
        }
        case Symbol::ACCESS: {
            build(st, ctx);
            u64 field = walker.next()->as_symbol().symbol->as_field().field;
            u64 offset{0};
            auto ltype = bin.left->get_type();
            if (ltype->is_struct()) {
                auto& fields = ltype->as_struct().pure->fields;
                for (u64 i = 0; i < fields.size(); ++i) {
                    if (i == field) {
                        offset = fields[i].offset;
                        break;
                    } // TODO Bitfields?
                }
            } else if (ltype->is_pointer() && ltype->as_pointer().t->is_struct()) {
                auto& fields = ltype->as_pointer().t->as_struct().pure->fields;
                for (u64 i = 0; i < fields.size(); ++i) {
                    if (i == field) {
                        offset = fields[i].offset;
                        break;
                    } // TODO Bitfields?
                }
                next({DEREFERENCE, c}, ltype->as_pointer().t);
            }
            return next({OFFSET, c, offset}, bin.t);
        }
        case Symbol::KWIMPORT: {
            walker.next(); // Ignore left
            build(st, ctx); // Build right
            return c;
        }
        default:
            logger::error() << bin.op << " not implemented for build(binary)" << logger::nend;
            return nullptr;
    }
}

ir_triple* ast_compiler::make() {
    ir_triple* t = new ir_triple;
    triples.push_back(t);
    return t;
}

ir_triple* ast_compiler::make(const ir_triple& triple, type* typ) {
    ir_triple* t = new ir_triple{triple};
    t->res_type = typ;
    triples.push_back(t);
    return t;
}

ir_triple* ast_compiler::next() {
    ir_triple* t = make();
    t->next = c->next;
    c->next = t;
    c = t;
    return t;
}

ir_triple* ast_compiler::next(ir_triple* triple) {
    triple->next = c->next;
    c->next = triple;
    c = triple;
    return triple;
}

ir_triple* ast_compiler::next(const ir_triple& triple, type* typ) {
    ir_triple* t = make(triple);
    t->res_type = typ;
    t->next = c->next;
    c->next = t;
    c = t;
    return t;
}

ir_triple* ast_compiler::jump() {
    ir_triple* t = make();
    fix_jump(t);
    return t;
}

ir_triple* ast_compiler::jump_to(ir_triple* other) {
    fix_jump(other);
    return other;
}

ir_triple* ast_compiler::ahead() {
    ir_triple* t = make();
    t->next = c->next;
    c->next = t;
    return t;
}

ir_triple* ast_compiler::ahead(const ir_triple& triple, type* typ) {
    ir_triple* t = make(triple);
    t->res_type = typ;
    t->next = c->next;
    c->next = t;
    return t;
}

void ast_compiler::move_to(ir_triple* before_begin, ir_triple* end, ir_triple* after_this) {
    ir_triple* begin = before_begin->next;
    ir_triple* after_end = end->next;
    ir_triple* after_after_this = after_this->next;
    before_begin->next = after_end;
    after_this->next = begin;
    end->next = after_after_this;
}

void ast_compiler::fix_jump(ir_triple* cond) {
    c->cond = cond;
    switch (c->op) {
        case ir_op::JUMP_RELATIVE: [[fallthrough]];
        default: 
            break;
        case ir_op::JUMP: [[fallthrough]];
        case ir_op::CALL: [[fallthrough]];
        case ir_op::CALL_CLOSURE:
            c->op1 = cond;
            break;
        case ir_op::IF_TRUE: [[fallthrough]];
        case ir_op::IF_FALSE:
            c->op2 = cond;
            break;
    }
}

void ast_compiler::fix_declarations(const std::vector<ast_compiler_ns::declaration>& decls) {
    for (auto& decl : decls) {
        offset_info[decl.entry] = decl;
    }
    
    // Reordering if needed comes here too
}

ir_op::code ast_compiler::grammar_symbol_to_ir_op(Grammar::Symbol sym) {
    using namespace ir_op;
    using namespace Grammar;
    
    switch (sym) {
        case Symbol::MULTIPLY_ASSIGN: [[fallthrough]];
        case Symbol::MULTIPLY: return MULTIPLY;
        case Symbol::GREATER: return GREATER;
        case Symbol::LESS: return LESS;
        case Symbol::ADD_ASSIGN: [[fallthrough]];
        case Symbol::ADD: return ADD;
        case Symbol::SUBTRACT_ASSIGN: [[fallthrough]];
        case Symbol::SUBTRACT: return SUBTRACT;
        case Symbol::POWER_ASSIGN: [[fallthrough]];
        case Symbol::POWER: return POWER;
        case Symbol::DIVIDE_ASSIGN: [[fallthrough]];
        case Symbol::DIVIDE: return DIVIDE;
        case Symbol::MODULO: return MODULO;
        case Symbol::AND_ASSIGN: [[fallthrough]];
        case Symbol::AND: return AND;
        case Symbol::OR_ASSIGN: [[fallthrough]];
        case Symbol::OR: return OR;
        case Symbol::XOR_ASSIGN: [[fallthrough]];
        case Symbol::XOR: return XOR;
        case Symbol::SHIFT_LEFT_ASSIGN: [[fallthrough]];
        case Symbol::SHIFT_LEFT: return SHIFT_LEFT;
        case Symbol::SHIFT_RIGHT_ASSIGN: [[fallthrough]];
        case Symbol::SHIFT_RIGHT: return SHIFT_RIGHT;
        case Symbol::ROTATE_LEFT_ASSIGN: [[fallthrough]];
        case Symbol::ROTATE_LEFT: return ROTATE_LEFT;
        case Symbol::ROTATE_RIGHT_ASSIGN: [[fallthrough]];
        case Symbol::ROTATE_RIGHT: return ROTATE_RIGHT;
        case Symbol::BIT_SET_ASSIGN: [[fallthrough]];
        case Symbol::BIT_SET: return SET_BIT;
        case Symbol::BIT_CLEAR_ASSIGN: [[fallthrough]];
        case Symbol::BIT_CLEAR: return CLEAR_BIT;
        case Symbol::BIT_TOGGLE_ASSIGN: [[fallthrough]];
        case Symbol::BIT_TOGGLE: return TOGGLE_BIT;
        case Symbol::BIT_CHECK: return BIT_SET; 
        case Symbol::EQUALS: return EQUALS;
        case Symbol::NOT_EQUALS: return NOT_EQUALS;
        case Symbol::GREATER_OR_EQUALS: return GREATER_EQUALS;
        case Symbol::LESS_OR_EQUALS: return LESS_EQUALS;
        case Symbol::CONCATENATE_ASSIGN: return CONCATENATE;
        default: return NOOP;
    }
}

ir_op::code ast_compiler::conversion_operator(type* from, type* to) {
    using namespace ir_op;
    if (from == to) {
        return NOOP;
    }
    if (from->is_primitive(etype_ids::FLOAT)) {
        if (to->is_primitive(etype_ids::DOUBLE)) {
            return CAST_FTD;
        } else if (to->flags & etype_flags::SIGNED) {
            return CAST_FTS;
        } else if (to->is_numeric()) {
            return CAST_FTU;
        } else {
            return NOOP;
        }
    } else if (from->is_primitive(etype_ids::DOUBLE)) {
        if (to->is_primitive(etype_ids::FLOAT)) {
            return CAST_DTF;
        } else if (to->flags & etype_flags::SIGNED) {
            return CAST_DTS;
        } else if (to->is_numeric()) {
            return CAST_DTU;
        } else {
            return NOOP;
        }
    } else if (from->flags & etype_flags::SIGNED) {
        if (to->is_primitive(etype_ids::FLOAT)) {
            return CAST_STF;
        } else if (to->is_primitive(etype_ids::DOUBLE)) {
            return CAST_STD;
        } else if (to->is_numeric() && !(to->flags & etype_flags::SIGNED)) {
            return CAST_STU;
        } else {
            return NOOP;
        }
    } else if (from->is_numeric()) {
        if (to->is_primitive(etype_ids::FLOAT)) {
            return CAST_UTF;
        } else if (to->is_primitive(etype_ids::DOUBLE)) {
            return CAST_UTD;
        } else if (to->flags & etype_flags::SIGNED) {
            return CAST_UTS;
        } else {
            return NOOP;
        }
    } else {
        return NOOP;
    }
}
