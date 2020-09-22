#include "frontend/compilers/ast_compiler.h"

#include "frontend/compiler.h"
#include "common/ast.h"
#include "common/symbol_table.h"
#include "common/type_table.h"
#include "common/logger.h"

ast_compiler::ast_compiler(compiler& c, nnmodule& mod, ast* node, symbol_table* st, symbol* sym) 
    : comp{c}, mod{mod}, root_node{node}, tt{mod.tt}, root_st{*mod.st}, st{*st}, root_sym{sym} {

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
                auto* sym = st.add_undefined(def->binary.left->tok->content, tt.TYPE, def);
                sym->variable.st = root_st.make_child(sym);
                
                // Make a new type for the struct, we'll pass it along to the upcoming compilation functions as-if it were a real type already
                type t{tt, 0, type_compound{{}}, false, false};
                type_supercompound sc{&t, false, false};
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
                sym->variable.defined = true;
                
                break;
            }
            case grammar::KW_UNION: {
                // Same as struct, but with unions
                ASSERT(!def->binary.left->is_zero(), "Name was placeholder");
                auto* sym = st.add_undefined(def->binary.left->tok->content, tt.TYPE, def);
                sym->variable.st = root_st.make_child(sym);
                
                type t{tt, 0, type_compound{{}}, false, false};
                type_supercompound sc{&t, false, false};
                type sct{tt, 0, type_type::UNION, sc, false, false};
                ast* nntype = ast::make_nntype({&sct}, def->tok, tt.TYPE); // Note it's not added to the type table
                
                sym->variable.value = nntype;
                sym->variable.t = tt.TYPE;
                
                auto sub_compiler = ast_compiler{comp, mod, def->binary.right, sym->variable.st, sym};
                sub_compiler.compile_block(def->binary.right, &ast_compiler::compile_struct);
                
                sc.comp = tt.add_compound(t.compound, false, false);
                nntype->nntype.t = tt.add_supercompound(sc, type_type::UNION, false, false);
                
                size_loop(nntype->nntype.t);
                sym->variable.defined = true;
                
                break;
            }
            case grammar::KW_ENUM: {
                // Same as struct, except the enum block has its own compilation function
                ASSERT(!def->binary.left->is_zero(), "Name was placeholder");
                auto* sym = st.add_undefined(def->binary.left->tok->content, tt.TYPE, def);
                sym->variable.st = root_st.make_child(sym);
                
                type t{tt, 0, type_compound{{}}, false, false};
                type_supercompound sc{&t, false, false};
                type sct{tt, 0, type_type::ENUM, sc, false, false};
                ast* nntype = ast::make_nntype({&sct}, def->tok, tt.TYPE); // Note it's not added to the type table
                
                sym->variable.value = nntype;
                sym->variable.t = tt.TYPE;
                
                auto sub_compiler = ast_compiler{comp, mod, def->binary.right, sym->variable.st, sym};
                sub_compiler.compile_enum(def->binary.right);
                
                sc.comp = tt.add_compound(t.compound, false, false);
                nntype->nntype.t = tt.add_supercompound(sc, type_type::ENUM, false, false);
                
                size_loop(nntype->nntype.t);
                sym->variable.defined = true;
                
                break;
            }
            case grammar::KW_TUPLE: {
                // Same as struct, except the tuple block has its own compilation function
                ASSERT(!def->binary.left->is_zero(), "Name was placeholder");
                auto* sym = st.add_undefined(def->binary.left->tok->content, tt.TYPE, def);
                sym->variable.st = nullptr;
                
                type t{tt, 0, type_compound{{}}, false, false};
                type_supercompound sc{&t, false, false};
                type sct{tt, 0, type_type::TUPLE, sc, false, false};
                ast* nntype = ast::make_nntype({&sct}, def->tok, tt.TYPE); // Note it's not added to the type table
                
                sym->variable.value = nntype;
                sym->variable.t = tt.TYPE;
                
                auto sub_compiler = ast_compiler{comp, mod, def->binary.right, sym->variable.st, sym};
                sub_compiler.compile_tuple(def->binary.right);
                
                sc.comp = tt.add_compound(t.compound, false, false);
                nntype->nntype.t = tt.add_supercompound(sc, type_type::ENUM, false, false);
                
                size_loop(nntype->nntype.t);
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
            nsym = st.make_and_add_placeholder(name->tok->content, tt.NONE_FUNCTION, def);
        } else {
            // Non-methods _always_ go in the root ST.
            // TODO fix scoping being the root_st instead of local
            nsym = root_st.make_and_add_placeholder(name->tok->content, tt.NONE_FUNCTION, def);
        }
        
        // All functions have their own scope, thus their own symbol table
        symbol_table* ftable = root_st.make_child(nsym);
        
        // The internal symbol table is distinct from the inner block symbol table
        // used for e64 values
        nsym->variable.st = root_st.make_child(nsym);
        
        // This type will eventually be filled with parameters and other shenanigans
        type t{tt, 0, type_function{}, false, false};
        
        // Actual function type
        type_superfunction sf{&t, {}, {}, false, false, root_st.make_child()};
        
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
            } else if (ctv->tt != ast_type::TYPE) {
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
        nsym->variable.compiletime = compiletime;
        
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
        
        sub_compiler.compile_block(body, &ast_compiler::compile_function); // After this returns entire function is done
        
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
            
            nsym->variable.defined = true;
        }

    } else {
        ASSERT(false, "Invalid def target");
    }
}

void ast_compiler::compile_block(ast* root, comp_func f) {
    ASSERT(root->tt == ast_type::BLOCK, "Node was not a block");
    
    for (auto node : root->block.elems) {
        if (node->tt == ast_type::UNARY && node->unary.sym == grammar::KW_DEF) {
            comp.compile_ast_task(&mod, node, &st, root_sym);
        } 
    }
    
    // We've set all nodes to compile, give them time to add themselves to the symbol table
    fiber::yield();
    // Names should not necessarily be complete now, but they're definitely "declared"
    
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
            ast_compiler sub_compiler{comp, mod, node, st.make_child(), root_sym};
            sub_compiler.compile_block(node, &ast_compiler::compile);
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
                    symbol* e64 = root_sym->variable.st->find(ename->tok->content).second; 
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
    }
}

void ast_compiler::compile_binary(ast* node) {
    
}

void ast_compiler::compile_compound(ast* node) {
    
}

void ast_compiler::compile_struct(ast* node) {
    
}

void ast_compiler::compile_enum(ast* node) {
    
}

void ast_compiler::compile_tuple(ast* node) {
    
}

void ast_compiler::compile_function(ast* node) {
    compile(node);
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

