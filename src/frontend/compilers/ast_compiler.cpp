#include "frontend/compilers/ast_compiler.h"

#include <algorithm>

#include "frontend/compiler.h"
#include "common/ast.h"
#include "common/symbol_table.h"
#include "common/type_table.h"
#include "common/logger.h"
#include "common/util.h"

ast_compiler::ast_compiler(compiler& c, nnmodule& mod, ast* node, symbol_table* st, symbol* sym) 
    : comp{c}, mod{mod}, root_node{node}, tt{mod.tt}, mod_st{*mod.st}, root_st{*st}, root_sym{sym} {

}

/* First compile function called, on the root of the AST */
void ast_compiler::compile_root() {
    switch (root_node->tt) {
        case ast_type::UNARY: // Can only be def statements
            ASSERT(root_node->unary.sym == grammar::KW_DEF, "Only definitions may be directly compiled");
            compile_def(root_node);
            break;
        case ast_type::BLOCK: // File scope code and definitions
            compile_block(root_node, &ast_compiler::compile);
            break;
        default:
            ASSERT(false, "Only blocks or unary defs may be directly compiled");
    }
}

/* Compiles a def statement. That is, works on function, struct, union, enum and tuple definitions */
void ast_compiler::compile_def(ast* root) {
    ASSERT(root->tt == ast_type::UNARY && root->unary.sym == grammar::KW_DEF, "Node was not unary def");
    
    ast* def = root->unary.node; // typelitdef
    
    if (def->tt == ast_type::BINARY) {
        switch (def->binary.sym) {
            case grammar::KW_STRUCT: { // Structs
                ASSERT(!def->binary.left->is_zero(), "Name was placeholder"); // TODO Allow placeholder names and anonymous structs
                // Add undefined symbol with struct name, for fibers fibers fibers
                auto* sym = root_st.add_undefined(def->binary.left->tok->content, tt.TYPE, def);
                sym->variable.st = mod_st.make_child(sym);
                
                // Make a new type for the struct, we'll pass it along to the upcoming compilation functions as-if it were a real type already
                type t{tt, 0, type_compound{{}}, false, false};
                type_supercompound sc{&t, false, false, sym->variable.st};
                type sct{tt, 0, type_type::STRUCT, sc, false, false};
                ast* nntype = ast::make_nntype({&sct}, def->tok, tt.TYPE); // Note it's not added to the type table
                
                sym->variable.value = nntype; // Unadded supercompound
                sym->variable.t = tt.TYPE; // The new symbol is a type type, not a real variable
                
                // Now that the name is in the ST, compile the struct statements (synchronously)
                auto sub_compiler = ast_compiler{comp, mod, def->binary.right, sym->variable.st, sym};
                sub_compiler.compile_block(def->binary.right, &ast_compiler::compile_struct);
                
                // We're done compiling the struct statements, so we make it a proper type
                // The added member variables are taken into account
                sc.comp = tt.add_compound(t.compound, false, false);
                nntype->nntype.t = tt.add_supercompound(sc, type_type::STRUCT, false, false);
                
                // Determine the size of the struct. This could be unknown due to using other structs recursively
                size_loop(nntype->nntype.t);
                // We're done!
                sym->variable.compiletime = true;
                sym->variable.defined = true;
                
                break;
            }
            case grammar::KW_UNION: {
                // Same as struct, but with unions
                ASSERT(!def->binary.left->is_zero(), "Name was placeholder");
                auto* sym = root_st.add_undefined(def->binary.left->tok->content, tt.TYPE, def);
                sym->variable.st = mod_st.make_child(sym);
                
                type t{tt, 0, type_compound{{}}, false, false};
                type_supercompound sc{&t, false, false, sym->variable.st};
                type sct{tt, 0, type_type::UNION, sc, false, false};
                ast* nntype = ast::make_nntype({&sct}, def->tok, tt.TYPE); // Note it's not added to the type table
                
                sym->variable.value = nntype;
                sym->variable.t = tt.TYPE;
                
                auto sub_compiler = ast_compiler{comp, mod, def->binary.right, sym->variable.st, sym};
                sub_compiler.compile_block(def->binary.right, &ast_compiler::compile_struct);
                
                sc.comp = tt.add_compound(t.compound, false, false);
                nntype->nntype.t = tt.add_supercompound(sc, type_type::UNION, false, false);
                
                size_loop(nntype->nntype.t);
                sym->variable.compiletime = true;
                sym->variable.defined = true;
                
                break;
            }
            case grammar::KW_ENUM: {
                // Same as struct, except the enum block has its own compilation function
                ASSERT(!def->binary.left->is_zero(), "Name was placeholder");
                auto* sym = root_st.add_undefined(def->binary.left->tok->content, tt.TYPE, def);
                sym->variable.st = mod_st.make_child(sym);
                
                type t{tt, 0, type_compound{{}}, false, false};
                type_supercompound sc{&t, false, false, sym->variable.st};
                type sct{tt, 0, type_type::ENUM, sc, false, false};
                ast* nntype = ast::make_nntype({&sct}, def->tok, tt.TYPE); // Note it's not added to the type table
                
                sym->variable.value = nntype;
                sym->variable.t = tt.TYPE;
                
                auto sub_compiler = ast_compiler{comp, mod, def->binary.right, sym->variable.st, sym};
                sub_compiler.compile_enum(def->binary.right);
                
                sc.comp = tt.add_compound(t.compound, false, false);
                nntype->nntype.t = tt.add_supercompound(sc, type_type::ENUM, false, false);
                
                size_loop(nntype->nntype.t);
                sym->variable.compiletime = true;
                sym->variable.defined = true;
                
                break;
            }
            case grammar::KW_TUPLE: {
                // Same as struct, except the tuple block has its own compilation function
                ASSERT(!def->binary.left->is_zero(), "Name was placeholder");
                auto* sym = root_st.add_undefined(def->binary.left->tok->content, tt.TYPE, def);
                sym->variable.st = nullptr;
                
                type t{tt, 0, type_compound{{}}, false, false};
                type_supercompound sc{&t, false, false, sym->variable.st};
                type sct{tt, 0, type_type::TUPLE, sc, false, false};
                ast* nntype = ast::make_nntype({&sct}, def->tok, tt.TYPE); // Note it's not added to the type table
                
                sym->variable.value = nntype;
                sym->variable.t = tt.TYPE;
                
                auto sub_compiler = ast_compiler{comp, mod, def->binary.right, sym->variable.st, sym};
                sub_compiler.compile_tuple(def->binary.right);
                
                sc.comp = tt.add_compound(t.compound, false, false);
                nntype->nntype.t = tt.add_supercompound(sc, type_type::ENUM, false, false);
                
                size_loop(nntype->nntype.t);
                sym->variable.compiletime = true;
                sym->variable.defined = true;
                
                break;
            }
            default:
                ASSERT(false, "Invalid def binary target");
                break;
        }
    } else if (def->tt == ast_type::COMPOUND) {
        // We're compiling a function
        // First gather all components in separate pointers for ease of use
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
        
        // Eventual function symbol
        symbol* nsym{nullptr}; // The function that results
        
        // If this is a method, then the compilation symbol parent must be a type (struct, union, etc)
        bool method = root_sym && root_sym->is_variable() && root_sym->variable.t->is_primitive(primitive_type::TYPE); 
        
        // TODO Can be something other than variable?
        if (method) {
            nsym = root_st.make_and_add_placeholder(name->tok->content, tt.NONE_FUNCTION, def);
        } else {
            // Non-methods _always_ go in the root ST.
            // TODO fix scoping being the root_st instead of local
            nsym = mod_st.make_and_add_placeholder(name->tok->content, tt.NONE_FUNCTION, def);
        }
        
        // All functions have their own scope, thus their own symbol table
        symbol_table* ftable = mod_st.make_child(nsym);
        
        // The internal symbol table is distinct from the inner block symbol table
        // used for e64 values
        nsym->variable.st = mod_st.make_child(nsym);
        
        // This type will eventually be filled with parameters and other shenanigans
        type t{tt, 0, type_function{}, false, false};
        
        // Actual function type
        type_superfunction sf{&t, {}, {}, false, false, nsym->variable.st};
        
        type sft{tt, 0, sf, false, false};
        
        ast* val = ast::make_nntype({&sft}, def->tok, tt.TYPE);
        
        // To get compiletime values from function
        ast_compiler sub_compiler{comp, mod, nullptr, ftable, nsym};
        
        // Modify this type from inside the function compile functions
        nsym->variable.value = val;
        
        auto& params = t.function.params;
        auto& rets = t.function.rets;
        
        auto& sparams = sf.params;
        auto& srets = sf.rets;
        
        auto add_param = 
        [&params, &sparams](grammar::symbol paramtype, const std::string& name, ast* value, bool binding, type* t, bool spread){
            ASSERT(paramtype == grammar::KW_REF || paramtype == grammar::KW_LET || paramtype == grammar::KW_VAR, "Invalid parameter type");
            
            // Create a param and sparam and then modify them
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
            param.spread = spread; // TODO Spread parameters should be arrays...
            
            return param;
        };
        
        auto add_ret = 
        [&rets, &srets](grammar::symbol rettype, const std::string& name, type* t) {
            ASSERT(rettype == grammar::KW_REF || rettype == grammar::KW_LET || rettype == grammar::KW_VAR, "Invalid return type");
        
            // Create a ret and sret and then modify them
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
            // Wait until the parent type has been defined
            // This works because methods have no influence on the parent type, so
            // it must eventually be defined, if nothing else breaks
            define_loop(root_sym);
            
            // The parent type is now properly defined, so the type is also valid
            add_param(grammar::KW_REF, "this", nullptr, false, root_sym->variable.value->nntype.t, false);
            params.back().thisarg = true;
            
            // Just like normal parameters, add "this" to the function st
            auto s = ftable->add_primitive("this", root_sym->variable.value->nntype.t, nullptr, nullptr, true, false, true);
            ASSERT(s != nullptr, "'this' already exists in the symbol table");
            s->variable.thisarg = true;
        }
        
        ASSERT(ftype->is_binary() && ftype->binary.sym == grammar::COLON, "Invalid function type ast");
        ASSERT(ftype->binary.left->is_compound(), "Params wasn't a compound");
        
        // Check every parameter declaration
        for (auto param : ftype->binary.left->compound.elems) {
            ASSERT(param->is_binary(), "Parameter wasn't binary");
            
            ast* nameval = param->binary.left;
            ASSERT(nameval->is_binary(), "Nameval wasn't binary");
            
            ast* ptype = param->binary.right;
            ASSERT(ptype->is_binary(), "Param type wasn't binary");
            
            grammar::symbol paramtype = param->binary.sym;
            std::string paramname{};
            
            if (nameval->binary.left->tt == ast_type::ZERO) {
                // Nothing here, placeholder parameter
            } else if (nameval->binary.left->tt == ast_type::IDENTIFIER) {
                paramname = nameval->tok->content;
            } else {
                ASSERT(false, "Invalid parameter name ast");
            }
            
            ast* value{nullptr};
            
            if (nameval->binary.right->tt != ast_type::NONE) {
                value = nameval->binary.right; // TODO Compile this?
            }
            
            bool binding = ptype->binary.sym == grammar::DCOLON;
            type* pt{nullptr};
            
            // Types must be compiletime
            sub_compiler.compile(ptype->binary.left);
            ast* ctv = get_compiletime_value(ptype->binary.left);
            
            if (!ctv) {
                // TODO Errors
                mod.errors.push_back({ctv, "Not a compiletime value"});
                pt = tt.ERROR_TYPE;
            } else if (!ctv->is_nntype()) {
                // TODO Errors
                mod.errors.push_back({ctv, "Not a type"});
                pt = tt.ERROR_TYPE;
            } else {
                pt = ctv->nntype.t;
            }
            
            bool spread = ptype->binary.right->is_zero() && ptype->binary.right->zero.sym == grammar::SPREAD;
            
            const auto& p = add_param(paramtype, paramname, value, binding, pt, spread);
            if (p.generic) {
                sft.sfunction.generic = true;
            }
            
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
        
        // Function type
        bool compiletime = returns->unary.sym == grammar::SRARROW;
        nsym->variable.compiletime_call = compiletime;
        
        bool any_infer{false};
        
        if (returns->unary.node->is_nntype()) {
            ASSERT(returns->unary.node->nntype.t == tt.INFER, "Single return was not infer");
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
                
                // Types must be compiletime
                sub_compiler.compile(rtype);
                ast* ctv = get_compiletime_value(rtype);
                
                if (!ctv) {
                    mod.errors.push_back({ctv, "Not a compiletime value"});
                    rt = tt.ERROR_TYPE;
                } else if (ctv->tt != ast_type::TYPE) {
                    mod.errors.push_back({ctv, "Not a type"});
                    rt = tt.ERROR_TYPE;
                } else {
                    rt = rtype->binary.left->compiled->nntype.t;
                }
                
                if (rt == tt.INFER) {
                    // A single infer only
                    any_infer = true;
                }
                
                add_ret(rettype, retname, rt);
                if (!retname.empty()) {
                    // TODO How does this work in practice?
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
            // If nothing is inferred, the function's type is complete, else we have to wait
            type* rf = tt.add_function(t.function, false, false);
            sf.function = rf;
            type* rcf = tt.add_superfunction(sf, false, false);
            nsym->variable.t = rcf;
            
            nsym->variable.defined = true;
        }
        
        ASSERT(body->is_block(), "Function body was not a block");
        
        // Set up goto fund, for lost gotos
        nsym->variable.gotos = ast::make_block({}, nullptr, tt.NONE);
        
        sub_compiler.compile_block(body, &ast_compiler::compile_function); // After this returns entire function is done
        
        if (nsym->variable.gotos->block.elems.count) {
            for (auto goto_ast : nsym->variable.gotos->block.elems) {
                ASSERT(goto_ast->is_unary(), "Goto statement was not unary");
                ASSERT(goto_ast->unary.node->is_iden(), "Goto value wasn't identifier");
                auto label = nsym->variable.st->get(goto_ast->unary.node->tok->content);
                if (!label) {
                    mod.errors.push_back({goto_ast, "Goto without label"});
                    continue;
                }
                goto_ast->unary.node->iden.s = label;
            }
        }
        
        nsym->variable.gotos->block.elems.head = nullptr; // These asts are not owned...
        delete nsym->variable.gotos;
        nsym->variable.gotos = nullptr;
    
        if (any_infer) {
            // Check and/or fix inferences
            bool had_error = false;
            if (t.function.rets.size() == 1) {
                if (t.function.rets[0].t == tt.INFER) {
                    // Void
                    t.function.rets[0].t = tt.U0;
                    t.function.rets[0].compiletime = true;
                } 
            } else {
                for (u64 i = 0; i < t.function.rets.size(); ++i) {
                    ret& r = t.function.rets[i];
                    if (r.t == tt.INFER) {
                        if (!had_error) {
                            had_error = true;
                            mod.errors.push_back({def, "Inferred function returns could not be determined"});
                        }
                        r.t = tt.ERROR_TYPE;
                    } 
                }
            }
            
            type* rf = tt.add_function(t.function, false, false);
            sf.function = rf;
            type* rcf = tt.add_superfunction(sf, false, false);
            nsym->variable.t = rcf;
            
            nsym->variable.compiletime = true;
            nsym->variable.defined = true;
        }

    } else {
        ASSERT(false, "Invalid def target");
    }
}

void ast_compiler::compile_block(ast* root) {
    compile_block(root, &ast_compiler::compile);
}

void ast_compiler::compile_block(ast* root, comp_func f) {
    ASSERT(root->tt == ast_type::BLOCK, "Node was not a block");
    
    bool need_yield = false;
    
    for (auto node : root->block.elems) {
        if (node->tt == ast_type::UNARY && node->unary.sym == grammar::KW_DEF) {
            comp.compile_ast_task(&mod, node, &root_st, root_sym);
            need_yield = true;
        } 
    }
    
    // We've set all nodes to compile, give them time to add themselves to the symbol table
    if (need_yield) {
        fiber::yield();
    }
    // Names should not necessarily be complete now, but they're definitely "declared"
    
    block_node = root;
    for (auto node : root->block.elems) {
        if (node->tt != ast_type::UNARY || node->unary.sym != grammar::KW_DEF) {
            (this->*f)(node);
        } 
    }
}

// Normal statements
void ast_compiler::compile(ast* node) {
    if (node->compiled) {
        return;
    }
    
    switch (node->tt) {
        case ast_type::NONE: [[fallthrough]];
        case ast_type::VALUE: [[fallthrough]];
        case ast_type::STRING: [[fallthrough]];
        case ast_type::TYPE: [[fallthrough]];
        case ast_type::IDENTIFIER:
            ASSERT(false, "Illegal node type was uncompiled");
            return;
        case ast_type::ZERO:
            compile_zero(node);
            break;
        case ast_type::UNARY:
            compile_unary(node);
            break;
        case ast_type::BINARY:
            compile_binary(node);
            break;
        case ast_type::COMPOUND:
            compile_compound(node);
            break;
        case ast_type::BLOCK:
            ast_compiler sub_compiler{comp, mod, node, root_st.make_child(), root_sym};
            sub_compiler.compile_block(node);
            return;
    }
    
    ASSERT(node->compiled != nullptr, "Node was not compiled after all");
}

void ast_compiler::compile_zero(ast* node) {
    auto& zero = node->zero;
    switch (zero.sym) {
        case grammar::KW_PLACEHOLDER: [[fallthrough]];
        case grammar::KW_THIS: [[fallthrough]];
        case grammar::KW_RAISE: [[fallthrough]];
        case grammar::KW_BREAK: [[fallthrough]];
        case grammar::KW_CONTINUE:
            node->compiled = node;
            break;
        default:
            logger::error() << node->to_simple_string();
            ASSERT(false, "Unknown zero node");
    }
}

void ast_compiler::compile_unary(ast* node) {
    auto& unary = node->unary;
    switch (unary.sym) {
        case grammar::KW_RETURN: {
            // Compare values to return types. Weak casting allowed
            // If return type is infer, then change the return type to whatever values are given here, now
            ASSERT(unary.node->is_block(), "Return without a block");
            if (!root_sym || !root_sym->is_variable() || !root_sym->variable.value->is_nntype() || 
                !root_sym->variable.value->nntype.t->is_superfunction()) {
                mod.errors.push_back({node, "return outside of function"});
                break;
            }
            std::vector<ast*> rets{};
            for (auto expr : unary.node->block.elems) {
                compile(expr);
                rets.push_back(expr->compiled);
            }
            type_function& ft = root_sym->variable.value->nntype.t->sfunction.function->function;
            if (root_sym->variable.infer_ret) {
                ft.rets.clear();
                for (auto ret : rets) {
                    ft.rets.push_back({ret->t, root_sym->variable.compiletime, false});
                }
                
                root_sym->variable.infer_ret = false;
            } else {
                u64 frets{0}, crets{0};
                while (frets < ft.rets.size() && crets < rets.size()) {
                    type* f = ft.rets[frets].t; // Function expected type
                    type* c = rets[crets]->t; // Return expression actual type
                    
                    if (tt.can_convert_weak(c, f)) {
                        if (f == tt.INFER) {
                            // Fix inference
                            ft.rets[frets].t = rets[crets]->t;
                        }
                        ++frets;
                        ++crets;
                    } else {
                        if (f == tt.E64) {
                            // TODO This is problematic, the insertion of a 0 value shifts the returns and keeps them out of sync with the vector
                            unary.node->block.elems.insert_before(rets[crets], ast::make_value({0}, nullptr, tt.E64));
                            ++frets;
                        } else {
                            // TODO Errors
                            mod.errors.push_back({rets[crets], "Return expression type does not match function return type"});
                            ++frets;
                            ++crets;
                        }
                    }
                }
                if (frets < ft.rets.size()) {
                    // TODO Errors
                    mod.errors.push_back({node, "Too many return values"});
                    break;
                } else if (crets < ft.rets.size()) {
                    mod.errors.push_back({node, "Not enough return values"});
                    break;
                }
                
                if (root_sym->variable.t->id == 0) { // Incomplete type, fixx
                    type* rf = tt.add_function(root_sym->variable.t->sfunction.function->function, false, false);
                    root_sym->variable.t->sfunction.function = rf;
                    type* rcf = tt.add_superfunction(root_sym->variable.t->sfunction, false, false);
                    root_sym->variable.t = rcf;
                    
                    root_sym->variable.defined = true; // Type completed, we're gucci
                }
            }
            node->compiled = node;
            break;
        }
        case grammar::KW_RAISE: {
            // Compare amount of errors to return type.
            ASSERT(unary.node->is_block(), "Raise without a block");
            if (!root_sym || !root_sym->is_variable() || !root_sym->variable.value->is_nntype() || 
                !root_sym->variable.value->nntype.t->is_superfunction()) {
                mod.errors.push_back({node, "raise outside of function"});
                break;
            }
            bool has_e64 = false;
            type_function& ft = root_sym->variable.value->nntype.t->sfunction.function->function;
            if (root_sym->variable.infer_ret) {
                mod.errors.push_back({node, "Cannot raise from fully inferred function"});
                break;
            }
            for (auto ret : ft.rets) {
                if (ret.t == tt.E64) {
                    has_e64 = true;
                    break;
                }
            }
            if (!has_e64) {
                mod.errors.push_back({node, "No e64 return available"});
                break;
            }
            
            if (unary.node->block.elems.count < 1) {
                mod.errors.push_back({node, "Not enough values in raise"});
            } else if (unary.node->block.elems.count > 2) {
                mod.errors.push_back({node, "Too many values in raise"});
            } else {
                auto ename = unary.node->block.elems.head;
                auto emesg = ename->next;
                
                if (!ename->is_iden() || ename->iden.s) {
                    compile(ename);
                    if (!tt.can_convert_weak(ename->compiled->t, tt.E64)) {
                        mod.errors.push_back({ename, "Value raised was not e64"});
                    }
                } else {
                    symbol* e64 = root_sym->variable.st->get(ename->tok->content); 
                    if (!e64) {
                        e64 = root_sym->variable.st->add_primitive(ename->tok->content, tt.E64, node, nullptr, true, true);
                    }
                    ename->iden.s = e64;
                    ename->compiled = ename;
                }
                
                if (emesg) {
                    compile(emesg);
                    if (!tt.can_convert_weak(emesg->compiled->t, tt.array_of(tt.C8))) {
                        mod.errors.push_back({emesg, "Value was not a c8 string"});
                    }
                }
            }
            node->compiled = node;
            break;
        }
        case grammar::KW_GOTO: {
            ASSERT(unary.node->is_iden(), "Goto without iden");
            auto label = root_sym->variable.st->get(unary.node->tok->content);
            if (!label) {
                root_sym->variable.gotos->block.elems.push_back(node);
            } else {
                unary.node->iden.s = label;
            }
            node->compiled = node;
            break;
        }
        case grammar::KW_LABEL: {
            ASSERT(unary.node->is_iden(), "Label without iden");
            auto label = root_sym->variable.st->get(unary.node->tok->content);
            if (label) {
                mod.errors.push_back({node, "Variable with this name already exists"});
            } else {
                root_sym->variable.st->add_label(unary.node->tok->content, node);
            }
            node->compiled = node;
            break;
        }
        case grammar::KW_DEFER: {
            compile(unary.node); // Compile whatever expression this is tbh
            block_node->block.at_end.push_front(unary.node);
            node->compiled = node;
            break;
        }
        case grammar::COLON: [[fallthrough]];
        case grammar::DCOLON : {
            // Only types?
            compile(unary.node);
            node->compiled = unary.node->compiled; // The value of this is just the value of the type
            node->compile_owned = false;
            break;
        }
        case grammar::KW_VAR: [[fallthrough]];
        case grammar::KW_LET: [[fallthrough]];
        case grammar::KW_REF: {
            ASSERT(unary.node->is_binary(), "Unary node is not a simplevardecl");
            auto& assignment = unary.node->binary;
            ASSERT(assignment.left->is_block(), "Declarations were not a block");
            auto& decls = assignment.left->block;
            
            std::vector<symbol*> declared_syms{};
            std::vector<ast*> values{};
            
            for (auto decl_block : decls.elems) {
                ASSERT(decl_block->is_binary(), "Declaration block was not a name-type binary");
                ASSERT(decl_block->binary.left->is_block(), "Declaration names were not a block");
                auto& names = decl_block->binary.left->block;
                auto block_type_ast = decl_block->binary.right;
            
                compile(block_type_ast);
                ast* ctv = get_compiletime_value(block_type_ast);
                type* block_type = nullptr;
                
                if (!ctv) {
                    mod.errors.push_back({ctv, "Not a compiletime value"});
                    block_type = tt.ERROR_TYPE;
                } else if (!ctv->is_nntype()) {
                    mod.errors.push_back({ctv, "Not a type"});
                    block_type = tt.ERROR_TYPE;
                } else {
                    block_type = ctv->nntype.t;
                }
                
                for (auto name : names.elems) {
                    symbol* sym = nullptr;
                    if (name->is_zero() && name->zero.sym == grammar::KW_PLACEHOLDER) {
                        sym = root_st.add_unnamed(block_type, decl_block, 
                                                  false, unary.sym == grammar::KW_LET, 
                                                  unary.sym == grammar::KW_REF, false, false);
                    } else {
                        ASSERT(name->is_iden(), "Name was not an identifier or a placeholder");
                        sym = new symbol{
                            name->tok->content, decl_block, symbol_variable{
                                block_type, nullptr, nullptr, false, 
                                unary.sym == grammar::KW_LET, 
                                unary.sym == grammar::KW_REF, // TODO Add checks for assignment to things without an address, const, etc
                                false, false, false, false
                            }
                        }; // Not added here to shadow previous declarations
                        if (!sym) {
                            mod.errors.push_back({name, "Variable already exists"});
                            sym = root_st.get(name->tok->content);
                        }
                    }
                    declared_syms.push_back(sym);
                }
            }
            
            bool last_is_compound{false};
            if (!assignment.right->is_none()) {
                ASSERT(assignment.right->is_block(), "Invalid assignment");
                // u64 i{0}, j{0};
                for (auto value : assignment.right->block.elems) {
                    compile(value);
                    
                    type* value_type = value->t;
                    
                    if (value_type->is_compound()) {
                        last_is_compound = true; // TODO Not good enough to prevent value overflow
                        for (u64 i = 0; i < value_type->compound.elems.size(); ++i) {
                            ast* selected_value = ast::make_binary({
                                grammar::OSELECT, value, 
                                ast::make_value({i}, value->tok, tt.U64)
                            }, value->tok, value_type->compound.elems[i]);
                            assignment.right->block.at_end.push_back(selected_value); // This ensures it'll be deleted
                            values.push_back(selected_value);
                        }
                    } else {
                        last_is_compound = false;
                        values.push_back(value);
                    }
                    // ++i; ++j;
                }
            }
            
            u64 i = 0;
            for (; i < declared_syms.size() && i < values.size(); ++i) {
                if (!tt.can_convert_weak(values[i]->t, declared_syms[i]->variable.t)) {
                    if (values[i]->t == tt.GENERIC_COMPOUND) {
                        u64 j = i;
                        for (; i < declared_syms.size(); ++i) {
                            declared_syms[i]->variable.value = values[j];
                            declared_syms[i]->variable.t = tt.GENERIC_UNKNOWN;
                            declared_syms[i]->variable.defined = true;
                        }
                        last_is_compound = true;
                    } else if (values[i]->t == tt.ERROR_COMPOUND) {
                        u64 j = i;
                        for (; i < declared_syms.size(); ++i) {
                            declared_syms[i]->variable.value = values[j];
                            declared_syms[i]->variable.t = tt.ERROR_TYPE;
                            declared_syms[i]->variable.defined = true;
                        }
                        last_is_compound = true;
                    } else {
                        mod.errors.push_back({values[i], conversion_error(values[i]->t, declared_syms[i]->variable.t)});
                    }
                } else {
                    declared_syms[i]->variable.value = values[i];
                    declared_syms[i]->variable.t = values[i]->t;
                    declared_syms[i]->variable.defined = true;
                }
            }
            
            if (i < declared_syms.size()) { // We ran out of values, zero the rest
                for (; i < declared_syms.size(); ++i) {
                    ast* selected_value = ast::make_value({0}, declared_syms[i]->decl->tok, declared_syms[i]->variable.t);
                    assignment.right->block.at_end.push_back(selected_value);
                    declared_syms[i]->variable.value = selected_value;
                    declared_syms[i]->variable.t = selected_value->t;
                    declared_syms[i]->variable.defined = true;
                }
            } else if (i < values.size()) { // We ran out of syms, is this allowed?
                if (!last_is_compound) {
                    mod.errors.push_back({assignment.right, "Too many values provided"});
                }
            }
            
            // Everything accounted for, simplify
            ast* simplified = ast::make_block({}, node->tok, tt.TYPELESS);
            for (i = 0; i < declared_syms.size(); ++i) {
                symbol* sym = declared_syms[i];
                simplified->block.elems.push_back(ast::make_unary({
                    grammar::KW_DEF, ast::make_iden({
                        sym
                    }, sym->decl->tok, sym->variable.t)
                }, sym->decl->tok, sym->variable.t));
            }
            
            node->compiled = simplified;
            
            break;
        }
        case grammar::KW_DELETE: {
            ASSERT(unary.node->is_block(), "delete was not on a block");
            auto& stmts = unary.node->block;
            
            for (auto stmt : stmts.elems) {
                compile(stmt);
                if (!stmt->compiled->t->is_pointer() && !stmt->compiled->t->is_array()) {
                    mod.errors.push_back({stmt, "Cannot delete this"});
                }
            }
            
            node->compiled = node;
            break;
        }
        case grammar::SPREAD: {
            compile(unary.node);
            ast* expr = unary.node->compiled;
            if (expr->t->is_supercompound()) {
                node->t = expr->t->scompound.comp;
            } else if (expr->t->is_compound()) {
                node->t = expr->t; // TODO Hmmmmm
            } else if (expr->t == tt.GENERIC_COMPOUND || expr->t == tt.GENERIC_UNKNOWN) {
                node->t = tt.GENERIC_COMPOUND;
            } else if (expr->t == tt.ERROR_COMPOUND || expr->t == tt.ERROR_TYPE) {
                node->t = tt.ERROR_COMPOUND;
            } else {
                mod.errors.push_back({unary.node, "Cannot spread"});
                node->t = tt.ERROR_COMPOUND;
            }
            node->compiletime = expr->compiletime;
            node->compiled = node;
            break; 
        }
        case grammar::INCREMENT: {
            compile(unary.node);
            ast* expr = unary.node->compiled;
            if (expr->t->is_primitive(primitive_type::SIGNED) || expr->t->is_primitive(primitive_type::UNSIGNED) || 
                expr->t->is_primitive(primitive_type::CHARACTER) || expr->t->is_pointer(pointer_type::NAKED) || expr->t->is_generic()) {
                
                node->t = tt.propagate_generic(expr->t);
            } else {
                mod.errors.push_back({unary.node, "Cannot increment"});
                node->t = expr->t; // TODO Reasonable assumption?
            }
            
            node->compiletime = expr->compiletime;
            node->compiled = node;
            break;
        }
        case grammar::DECREMENT: {
            compile(unary.node);
            ast* expr = unary.node->compiled;
            if (expr->t->is_primitive(primitive_type::SIGNED) || expr->t->is_primitive(primitive_type::UNSIGNED) || 
                expr->t->is_primitive(primitive_type::CHARACTER) || expr->t->is_pointer(pointer_type::NAKED) || expr->t->is_generic()) {
                
                node->t = tt.propagate_generic(expr->t);
            } else {
                mod.errors.push_back({unary.node, "Cannot decrement"});
                node->t = expr->t; // TODO Reasonable assumption?
            }
                
            node->compiletime = expr->compiletime;
            node->compiled = node;
            break;
        }
        case grammar::SUB: {
            compile(unary.node);
            ast* expr = unary.node->compiled;
            if (expr->t->is_primitive(primitive_type::SIGNED) || expr->t->is_primitive(primitive_type::UNSIGNED)) {
                
                node->t = tt.get_signed(expr->t->primitive.bits);
            } else if (expr->t->is_primitive(primitive_type::FLOATING)) {
                node->t = expr->t;
            } else if (expr->t->is_generic()) {
                node->t = tt.propagate_generic(expr->t);
            } else {
                mod.errors.push_back({unary.node, "Cannot negate"});
                node->t = tt.ERROR_TYPE; // TODO Reasonable assumption?
            }
            
            node->compiletime = expr->compiletime;
            node->compiled = node;
            break;
        }
        case grammar::INFO: {
            compile(unary.node);
            ast* expr = unary.node->compiled;
            if (expr->t->is_array()) {
                node->compiletime = expr->t->array.sized;
                node->t = tt.U64; // Length of array
            } else if (expr->t == tt.ANY) {
                node->t = tt.TYPE;
            } else if (expr->t == tt.TYPE) {
                // TODO Type info structs
                logger::warn() << "Type info structs not implemented";
            } else if (expr->t->is_generic()) {
                node->t = tt.propagate_generic(expr->t);
            } else {
                mod.errors.push_back({unary.node, "Cannot negate"});
                node->t = tt.ERROR_TYPE; // TODO Reasonable assumption?
            }
            
            node->compiled = node;
            break;
        }
        case grammar::NOT: {
            compile(unary.node);
            ast* expr = unary.node->compiled;
            if (expr->t->is_primitive(primitive_type::SIGNED) || expr->t->is_primitive(primitive_type::UNSIGNED) || 
                expr->t->is_primitive(primitive_type::BOOLEAN) || expr->t->is_generic()) {
                
                node->t = tt.propagate_generic(expr->t);
            } else {
                mod.errors.push_back({unary.node, "Cannot apply not"});
                node->t = expr->t; // TODO Reasonable assumption?
            }
            
            node->compiletime = expr->compiletime;
            node->compiled = node;
            break;
        }
        case grammar::LNOT: {
            compile(unary.node);
            ast* expr = unary.node->compiled;
            if (tt.can_convert_strong(expr->t, tt.U1)) {
                
                node->t = tt.U1;
            } else {
                mod.errors.push_back({unary.node, "Cannot apply logical not"});
                node->t = tt.U1; // TODO Reasonable assumption?
            }
                
            node->compiletime = expr->compiletime;
            node->compiled = node;
            break;
        }
        case grammar::AT: {
            compile(unary.node);
            ast* expr = unary.node->compiled;
            if (expr->t->is_pointer() && expr->t->pointer.tt != pointer_type::WEAK) {
                
                node->t = expr->t->pointer.at;
            } else if (expr->t == tt.TYPE) {
                if (!expr->compiletime) {
                    mod.errors.push_back({unary.node, "Dereferencing of types only available at compiletime"});
                }
                node->t = tt.TYPE;
            } else {
                mod.errors.push_back({unary.node, "Cannot dereference"});
                node->t = tt.ERROR_TYPE;
            }
            
            node->compiletime = expr->compiletime;
            node->compiled = node;
            break;
        }
        case grammar::POINTER: {
            compile(unary.node);
            ast* expr = unary.node->compiled;
            if (expr->t == tt.TYPE && expr->compiletime) {
                node->t = tt.TYPE;
            } else {
                // TODO Some constructs cannot be pointered to, not addressable?
                node->t = tt.pointer_to(expr->t);
            }
            
            node->compiletime = expr->compiletime;
            node->compiled = node;
            break;
        }
        case grammar::WEAK_PTR: {
            compile(unary.node);
            ast* expr = unary.node->compiled;
            if (expr->t == tt.TYPE && expr->compiletime) {
                node->t = tt.TYPE;
            } else {
                mod.errors.push_back({unary.node, "Cannot convert to weak pointer, cast from shared pointer instead"});
                node->t = tt.pointer_to(expr->t, pointer_type::WEAK);
            }
            
            node->compiletime = expr->compiletime;
            node->compiled = node;
            break;
        }
        case grammar::OBRACK: {
            compile(unary.node);
            ast* expr = unary.node->compiled;
            if (expr->t == tt.TYPE && expr->compiletime) {
                node->t = tt.TYPE;
            } else {
                mod.errors.push_back({unary.node, "Cannot have empty index"});
                node->t = expr->t->is_array() ? expr->t->array.at : tt.ERROR_TYPE;
            }
            
            node->compiletime = expr->compiletime;
            node->compiled = node;
            break;
        }
        case grammar::ADD: {
            compile(unary.node);
            ast* expr = unary.node->compiled;
            if (expr->t == tt.TYPE && expr->compiletime) {
                node->t = tt.TYPE;
            } else {
                mod.errors.push_back({unary.node, "Cannot convert to shared pointer, use new instead"});
                node->t = tt.pointer_to(expr->t, pointer_type::WEAK);
            }
            
            node->compiletime = expr->compiletime;
            node->compiled = node;
            break;
        }
        case grammar::KW_TYPEOF: {
            compile(unary.node);
            node->t = tt.TYPE;
            
            node->compiletime = true;
            node->compiled = node;
            break;
        }
        case grammar::KW_SIZEOF: {
            compile(unary.node);
            node->t = tt.U64;
            
            node->compiletime = true;
            node->compiled = node;
            break;
        }
        case grammar::KW_CONST: {
            compile(unary.node);
            ast* expr = unary.node->compiled;
            if (expr->t == tt.TYPE && expr->compiletime) {
                node->t = tt.TYPE;
            } else {
                mod.errors.push_back({unary.node, "Cannot convert to const"});
                node->t = tt.reflag(expr->t, true, expr->t->volat_flag);
            }
            
            node->compiletime = expr->compiletime;
            node->compiled = node;
            break;
        }
        case grammar::KW_VOLAT: {
            compile(unary.node);
            ast* expr = unary.node->compiled;
            if (expr->t == tt.TYPE && expr->compiletime) {
                node->t = tt.TYPE;
            } else {
                mod.errors.push_back({unary.node, "Cannot convert to volat"});
                node->t = tt.reflag(expr->t, expr->t->const_flag, true);
            }
            
            node->compiletime = expr->compiletime;
            node->compiled = node;
            break;
        }
        case grammar::OPAREN: {
            compile(unary.node); // Easy every time
            node->t = unary.node->compiled->t;
            node->compiletime = unary.node->compiled->compiletime;
            node->compiled = unary.node->compiled;
            node->compile_owned = false;
            break;
        }
        default:
            logger::error() << unary.sym << " is not a valid unary symbol";
            ASSERT(false, "Invalid unary symbol");
            break;
    }
}

void ast_compiler::compile_binary(ast* node) {
    (void) node;
}

void ast_compiler::compile_compound(ast* node) {
    (void) node;
}

void ast_compiler::compile_struct(ast* node) {
    (void) node;
}

void ast_compiler::compile_enum(ast* node) {
    (void) node;
}

void ast_compiler::compile_tuple(ast* node) {
    (void) node;
}

void ast_compiler::compile_function(ast* node) {
    compile(node);
}

void ast_compiler::compile_iden(ast* node) {
    (void) node;
}

void ast_compiler::compile_function_call(ast* node, ast* first) {
    (void) node;
    (void) first;
}

void ast_compiler::compile_dot_expression(ast* node, bool allow_star) {
    ASSERT(node->is_binary(), "Dot expression was not binary");
    
    std::vector<ast*> nodes{node};
    ast* current{node};
    
    do {
        if (!current->binary.right->is_binary() || 
            current->binary.right->binary.sym != grammar::PERIOD) {
            break;
        }
        nodes.push_back(current->binary.right);
        current = current->binary.right;
    } while (true);
    
    ast* last{nullptr};
    ast* last_value{nullptr};
    
    for (auto it = nodes.rbegin(); it != nodes.rend(); ++it) {
        ast* dot_node = *it;
        auto& binary = dot_node->binary;
        if (!last) {
            // First entry
            compile(binary.left);
            last = binary.left;
        }
        
        ast* iden = binary.right->get_leftmost(); // TODO Does this always work?
        if (!iden->is_iden()) {
            // This is allowed if: This is the last element, and it's a star, and stars are allowed
            if ((it + 1) == nodes.rend() && 
                iden->is_zero() && iden->zero.sym == grammar::MUL && allow_star) {
                dot_node->compiled = dot_node;
                dot_node->compiletime = true;
                break;
            }
                
            mod.errors.push_back({iden, "Expecting identifier"});
            node->t = tt.ERROR_TYPE;
            node->compiled = node;
            return;
        }
        
        type* last_t = last->compiled->t;
        std::string& iden_name = iden->tok->content;
        symbol* found {nullptr};
        if (last_t->is_supertype()) {
            last_value = last;
            // Supertypes must have all their members defined by now at least
            if (last_t->is_superfunction()) {
                found = last_t->sfunction.st->get(iden_name);
            } else if (last_t->is_supercompound()) {
                found = last_t->scompound.st->get(iden_name);
            }
        } else if (last_t == tt.TYPE && last->compiled->compiletime) {
            ast* rt = get_compiletime_value(last);
            if (!rt->is_nntype() || !rt->nntype.t->is_supertype()) {
                mod.errors.push_back({last, "Invalid type"});
                node->t = tt.ERROR_TYPE;
                node->compiled = node;
                return;
            }
            
            if (rt->nntype.t->is_superfunction()) {
                found = rt->nntype.t->sfunction.st->get(iden_name);
            } else if (rt->nntype.t->is_supercompound()) {
                found = rt->nntype.t->scompound.st->get(iden_name);
            }
        } else if (last->compiled->is_iden()) {
            if (last->compiled->iden.s->is_namespace()) {
                found = last->compiled->iden.s->namespace_.st->get(iden_name);
            } else if (last->compiled->iden.s->is_module()) {
                found = last->compiled->iden.s->mod.mod->st->get(iden_name);
            }
        }
        
        if (!found) {
            last_value = last;
            found = root_st.find(iden_name).second;
        }
        
        if (!found && last_t->is_pointer()) {
            while (last_t->is_pointer()) {
                last_t = last_t->pointer.at;
            }
            
            if (last_t->is_supertype()) {
                // Supertypes must have all their members defined by now at least
                if (last_t->is_superfunction()) {
                    found = last_t->sfunction.st->get(iden_name);
                } else if (last_t->is_supercompound()) {
                    found = last_t->scompound.st->get(iden_name);
                }
            }
        }
        
        if (!found) {
            mod.errors.push_back({iden, "Cannot find identifier"});
            node->t = tt.ERROR_TYPE;
            node->compiled = node;
            return;
        }
        
        iden->iden.s = found;
        iden->compiled = iden;
        iden->compiletime = last->compiled->compiletime && (found->is_variable() ? found->variable.compiletime : true);
        
        if (binary.right->is_binary() && binary.right->binary.sym == grammar::OPAREN) { // Function call
            compile_function_call(binary.right, last_value);
            last_value = nullptr;
        } else {
            compile(binary.right);
        }
        
        last = dot_node;
        dot_node->compiled = dot_node;
        dot_node->t = binary.right->compiled->t;
        dot_node->compiletime = binary.right->compiled->compiletime;
    }
    
    node->t = node->binary.right->t;
    node->compiletime = node->binary.right->compiletime;
    node->compiled = node;
    
    return;
}

ast* ast_compiler::get_compiletime_value(ast* node) {    
    if (node->compiled && node->compiletime) {
        return node->compiled;
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

std::string ast_compiler::conversion_error(type* from, type* to) {
    return ss::get() << "Cannot convert " << from->to_string(true) << " to " << to->to_string(true) << ss::end();
}


