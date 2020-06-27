#include "frontend/compilers/ast_compiler.h"

#include "frontend/compiler.h"
#include "common/ast.h"
#include "common/symbol_table.h"
#include "common/type_table.h"
#include "common/logger.h"

ast_compiler::ast_compiler(compiler& c, nnmodule& mod, ast* node) 
    : comp{c}, mod{mod}, root_node{node}, tt{mod.tt}, root_st{*mod.st} {

        
}

void ast_compiler::compile_root(ast* root, symbol_table* st, symbol* sym) {
    switch (root->tt) {
        case ast_type::UNARY:
            ASSERT(root->unary.sym == grammar::KW_DEF, "Only definitions may be directly compiled");
            compile_def(root, st, sym);
            break;
        case ast_type::BLOCK:
            compile_block(root, st, nullptr, &ast_compiler::compile);
            break;
        default:
            ASSERT(false, "Only blocks or unary defs may be directly compiled");
    }
}

void ast_compiler::compile_def(ast* root, symbol_table* st, symbol* sym) {
    ASSERT(root->tt == ast_type::UNARY && root->unary.sym == grammar::KW_DEF, "Node was not unary def");
    
    ast* def = root->unary.node; // typelitdef
    
    if (def->tt == ast_type::BINARY) {
        switch (def->binary.sym) {
            case grammar::KW_STRUCT: {
                ASSERT(!def->binary.left->is_zero(), "Name was placeholder");
                auto* sym = st->add_undefined(def->binary.left->tok->content, tt.TYPE, def);
                sym->variable.st = root_st.make_child(sym);
                
                type t{tt, 0, type_compound{{}}, false, false};
                ast* nntype = ast::make_nntype({&t}, def->binary.left->tok, tt.TYPE);
                sym->variable.value = nntype;
                sym->variable.t = tt.TYPE;
                
                compile_block(def->binary.right, sym->variable.st, sym, &ast_compiler::compile_struct);
                
                auto comp = type_supercompound{tt.add_compound(t.compound, false, false)};
                nntype->nntype.t = tt.add_supercompound(comp, type_type::STRUCT, false, false);
                
                size_loop(nntype->nntype.t);
                sym->variable.defined = true;
                
                break;
            }
            case grammar::KW_UNION: {
                ASSERT(!def->binary.left->is_zero(), "Name was placeholder");
                auto* sym = st->add_undefined(def->binary.left->tok->content, tt.TYPE, def);
                sym->variable.st = root_st.make_child(sym);
                
                type t{tt, 0, type_compound{{}}, false, false};
                ast* nntype = ast::make_nntype({&t}, def->binary.left->tok, tt.TYPE);
                sym->variable.value = nntype;
                sym->variable.t = tt.TYPE;
                
                compile_block(def->binary.right, sym->variable.st, sym, &ast_compiler::compile_struct);
                
                auto comp = type_supercompound{tt.add_compound(t.compound, false, false)};
                nntype->nntype.t = tt.add_supercompound(comp, type_type::UNION, false, false);
                
                size_loop(nntype->nntype.t);
                sym->variable.defined = true;
                
                break;
            }
            case grammar::KW_ENUM: {
                ASSERT(!def->binary.left->is_zero(), "Name was placeholder");
                auto* sym = st->add_undefined(def->binary.left->tok->content, tt.TYPE, def);
                sym->variable.st = root_st.make_child(sym);
                
                type t{tt, 0, type_compound{{}}, false, false};
                ast* nntype = ast::make_nntype({&t}, def->binary.left->tok, tt.TYPE);
                sym->variable.value = nntype;
                sym->variable.t = tt.TYPE;
                
                compile_enum(def->binary.right, sym->variable.st, sym);
                
                auto comp = type_supercompound{tt.add_compound(t.compound, false, false)};
                nntype->nntype.t = tt.add_supercompound(comp, type_type::ENUM, false, false);
                
                size_loop(nntype->nntype.t);
                sym->variable.defined = true;
                
                break;
            }
            case grammar::KW_TUPLE: {
                ASSERT(!def->binary.left->is_zero(), "Name was placeholder");
                auto* sym = st->add_undefined(def->binary.left->tok->content, tt.TYPE, def);
                sym->variable.st = nullptr;
                
                type t{tt, 0, type_compound{{}}, false, false};
                ast* nntype = ast::make_nntype({&t}, def->binary.left->tok, tt.TYPE);
                sym->variable.value = nntype;
                sym->variable.t = tt.TYPE;
                
                compile_tuple(def->binary.right, st, sym);
                
                auto comp = type_supercompound{tt.add_compound(t.compound, false, false)};
                nntype->nntype.t = tt.add_supercompound(comp, type_type::TUPLE, false, false);
                
                size_loop(nntype->nntype.t);
                sym->variable.defined = true;
                
                break;
            }
            default:
                ASSERT(false, "Invalid def binary target");
                break;
        }
    } else if (def->tt == ast_type::COMPOUND) {
        ASSERT(def->compound.elems.head, "Function didn't have anything");
        ast* capture = def->compound.elems.head;
        (void) capture; // TODO Do not ignore this
        
        ASSERT(capture->next, "Function had no name at all");
        ast* name = capture->next;
        
        ASSERT(!name->is_zero(), "Function was nameless");
        ASSERT(name->next, "Function had no type");
        ast* ftype = name->next;
        
        ASSERT(ftype->next, "Function had no body");
        ast* body = ftype->next;
        
        symbol* nsym{nullptr};
        
        bool method = sym && sym->is_variable() && sym->variable.t->is_primitive(primitive_type::TYPE); 
        
        // TODO Can be something other than variable?
        if (method) {
            nsym = st->make_and_add_placeholder(name->tok->content, tt.NONE_FUNCTION, def);
        } else {
            nsym = root_st.make_and_add_placeholder(name->tok->content, tt.NONE_FUNCTION, def);
        }
        
        symbol_table* ftable = nsym->variable.st = root_st.make_child(nsym);
        
        type t{tt, 0, type_function{}, false, false};
        ast val{ast_type::TYPE, def->tok, tt.TYPE};
        val.nntype = ast_nntype{&t};
        val.compiled = &val;
        
        nsym->variable.value = &val;
        
        auto& params = t.function.params;
        auto& rets = t.function.rets;
        
        type_superfunction sf{nullptr, {}, {}, root_st.make_child()};
        
        auto& sparams = sf.params;
        auto& srets = sf.rets;
        
        auto add_param = 
        [&params, &sparams](grammar::symbol paramtype, const std::string& name, ast* value, bool binding, type* t, bool spread){
            ASSERT(paramtype == grammar::KW_REF || paramtype == grammar::KW_LET || paramtype == grammar::KW_VAR, "Invalid parameter type");
            
            auto& param = params.emplace_back();
            auto& sparam = sparams.emplace_back();
            
            switch (paramtype) {
                case grammar::KW_REF:
                    param.reference = true;
                    break;
                case grammar::KW_LET:
                    param.compiletime = true;
                    break;
                default: break;
            }
            
            sparam.name = name;
            sparam.defaulted = value != nullptr;
            sparam.value = value;
            
            param.binding = binding;
            param.generic = t->is_special(special_type::GENERIC);
            param.t = t;
        };
        
        auto add_ret = 
        [&rets, &srets](grammar::symbol rettype, const std::string& name, type* t) {
            ASSERT(rettype == grammar::KW_REF || rettype == grammar::KW_LET || rettype == grammar::KW_VAR, "Invalid return type");
        
            auto& ret = rets.emplace_back();
            auto& sret = srets.emplace_back();
            
            switch (rettype) {
                case grammar::KW_REF:
                    ret.reference = true;
                    break;
                case grammar::KW_LET:
                    ret.compiletime = true;
                    break;
                default: break;
            }
            
            sret.name = name;
            ret.t = t;
        };
        
        if (method) { // THISARG
            define_loop(sym); // Wait until the parent type has been defined
            
            add_param(grammar::KW_REF, "this", nullptr, false, sym->variable.value->nntype.t, false);
            params.back().thisarg = true;
            
            auto s = ftable->add_primitive("this", sym->variable.value->nntype.t, nullptr, nullptr, true, false, true);
            ASSERT(s != nullptr, "'this' already exists in the symbol table");
            s->variable.thisarg = true;
        }
        
        ASSERT(ftype->is_binary() && ftype->binary.sym == grammar::COLON, "Invalid function type ast");
        ASSERT(ftype->binary.left->is_compound(), "Params wasn't a compound");
        
        for (auto param : ftype->binary.left->compound.elems) {
            ASSERT(param->is_binary(), "Parameter wasn't binary");
            
            ast* nameval = param->binary.left;
            ASSERT(nameval->is_binary(), "Nameval wasn't binary");
            
            ast* ptype = param->binary.right;
            ASSERT(ptype->is_binary(), "Param type wasn't binary");
            
            grammar::symbol paramtype = param->binary.sym;
            std::string paramname{};
            
            if (nameval->binary.left->tt == ast_type::ZERO) {
                // Nothing here
            } else if (nameval->binary.left->tt == ast_type::IDENTIFIER) {
                paramname = nameval->tok->content;
            } else {
                ASSERT(false, "Invalid parameter name ast");
            }
            
            ast* value{nullptr};
            
            if (nameval->binary.right->tt != ast_type::NONE) {
                value = nameval->binary.right;
            }
            
            bool binding = ptype->binary.sym == grammar::DCOLON;
            type* pt{nullptr};
            
            get_compiletime_value(ptype->binary.left, ftable, nsym);
            
            if (ptype->binary.left->compiled->tt != ast_type::TYPE) {
                mod.errors.push_back({ptype->binary.left, "Not a type"});
                pt = tt.ERROR_TYPE;
            } else {
                pt = ptype->binary.left->compiled->nntype.t;
            }
            
            bool spread = ptype->binary.right->is_zero() && ptype->binary.right->zero.sym == grammar::SPREAD;
            
            add_param(paramtype, paramname, value, binding, pt, spread);
            
            if (!paramname.empty()) {
                auto ps = ftable->add_primitive(paramname, pt, value, param, true, paramtype == grammar::KW_LET, paramtype == grammar::KW_REF);
                if (!ps) {
                    // TODO Name exists?
                    mod.errors.push_back({nameval, "Parameter name already exists"});
                }
            }
        }
        
        ASSERT(ftype->binary.right->is_unary(), "Rets wasn't unary");
        ast* returns = ftype->binary.right;
        
        bool compiletime = returns->unary.sym == grammar::SRARROW;
        
        bool any_infer{false};
        
        if (returns->unary.node->is_nntype()) {
            // Infer
            nsym->variable.infer_ret = true;
            any_infer = true;
        } else {
            for (auto ret : returns->unary.node->compound.elems) {
                ASSERT(ret->is_binary(), "Return wasn't binary");
                
                grammar::symbol rettype = ret->binary.sym;
                
                std::string retname{};
                
                ast* name = ret->binary.left;
                if (name->is_zero() && name->zero.sym == grammar::KW_PLACEHOLDER) {
                    // Nothing
                } else if (name->is_iden()) {
                    retname = name->tok->content;
                } else {
                    ASSERT(false, "Invalid name");
                }
                
                type* rt{nullptr};
                ast* rtype = ret->binary.right;
                
                get_compiletime_value(rtype, ftable, nsym);
                
                if (rtype->binary.left->compiled->tt != ast_type::TYPE) {
                    mod.errors.push_back({rtype->binary.left, "Not a type"});
                    rt = tt.ERROR_TYPE;
                } else {
                    rt = rtype->binary.left->compiled->nntype.t;
                }
                
                if (rt == tt.INFER) {
                    any_infer = true;
                }
                
                add_ret(rettype, retname, rt);
                if (!retname.empty()) {
                    auto rs = ftable->add_primitive(retname, rt, ret, nullptr);
                    if (!rs) {
                        // TODO Name exists
                        mod.errors.push_back({name, "Return name already exists"});
                    } else {
                        rs->variable.is_return = true;
                    }
                }
            }
        }
        
        if (!any_infer) {
            nsym->variable.defined = true;
            
            type* rf = tt.add_function(t.function, false, false);
            sf.function = rf;
            type* rcf = tt.add_superfunction(sf, false, false);
            nsym->variable.t = rcf;
        }
        
        ASSERT(body->is_block(), "Function body was not a block");
        
        compile_block(body, ftable, nsym, &ast_compiler::compile_function); // After this returns entire function is done
        
        if (!any_infer) {
            bool had_error = false;
            for (u64 i = 0; i < t.function.rets.size(); ++i) {
                ret& r = t.function.rets[i];
                ret_info& sr = sf.rets[i];
                if (r.t == tt.INFER) {
                    if (!had_error) {
                        had_error = true;
                        mod.errors.push_back({def, "Inferred function returns could not be determined"});
                    }
                    r.t = tt.ERROR_TYPE;
                } 
            }
            
            nsym->variable.defined = true;
            
            type* rf = tt.add_function(t.function, false, false);
            sf.function = rf;
            type* rcf = tt.add_superfunction(sf, false, false);
            nsym->variable.t = rcf;
        }
        
        
    } else {
        ASSERT(false, "Invalid def target");
    }
}

void ast_compiler::compile_block(ast* root, symbol_table* st, symbol* sym, comp_func f) {
    ASSERT(root->tt == ast_type::BLOCK, "Node was not a block");
    
    for (auto node : root->block.elems) {
        if (node->tt == ast_type::UNARY && node->unary.sym == grammar::KW_DEF) {
            comp.compile_ast_task(node, st, sym);
        } 
    }
    
    // We've set all nodes to compile, give them time to add themselves to the symbol table
    fiber::yield();
    // Names should not necessarily be complete now, but they're definitely "declared"
    
    for (auto node : root->block.elems) {
        if (node->tt != ast_type::UNARY || node->unary.sym != grammar::KW_DEF) {
            (this->*f)(node, st, sym);
        } 
    }
}

void ast_compiler::compile(ast* node, symbol_table* st, symbol*) {
    
}

void ast_compiler::compile_struct(ast* node, symbol_table* st, symbol* sym) {
    
}

void ast_compiler::compile_enum(ast* node, symbol_table* st, symbol* sym) {
    
}

void ast_compiler::compile_tuple(ast* node, symbol_table* st, symbol* sym) {
    
}

void ast_compiler::compile_function(ast* node, symbol_table* st, symbol* sym) {
    
}

ast* ast_compiler::get_compiletime_value(ast* node, symbol_table* st, symbol* sym) {
    if (node->compiled) {
        return node->compiled;
    }
    
    logger::error() << "get_compiletime_value() not implemented for " << node->to_simple_string() 
                    << " ~ " << token::text_between(node->get_leftmost_token(), node->get_rightmost_token());
    ASSERT(false, "Not implemented");
    
    switch (node->tt) {
        case ast_type::ZERO: [[fallthrough]];
        case ast_type::UNARY: [[fallthrough]];
        case ast_type::BINARY: 
            // TODO ???
            break;
        default:
            return node;
    }
    
    return nullptr;
}

void ast_compiler::size_loop(type* t) {
    ASSERT(fiber::this_fiber() != nullptr, "Code needs to be called from inside fiber");
    
    u8 tries = 10; // TODO heuristcsss
    bool stall = false;
    while (--tries && !t->sized && !t->set_size()) {
        fiber::yield(stall);
        stall = true;
        
    }
    if (!tries) {
        fiber::crash(); // TODO :(
    }
}

void ast_compiler::define_loop(symbol* sym) {
    ASSERT(fiber::this_fiber() != nullptr, "Code needs to be called from inside fiber");
    ASSERT(sym->tt == symbol_type::VARIABLE, "Only variables can be undefined");
    
    
    u8 tries = 10; // TODO heuristcsss
    bool stall = false;
    while (--tries && !sym->variable.defined) {
        fiber::yield(stall);
        stall = true;
    }
    if (!tries) {
        fiber::crash(); // TODO :(
    }
}

