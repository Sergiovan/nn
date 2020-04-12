#include "common/symbol_table.h"
#include "common/type.h"

symbol::symbol() : tt{symbol_type::LABEL}, name{}, owner{nullptr}, label{} {
    
}

symbol::symbol(const symbol& o) {
    tt = o.tt;
    name = o.name;
    owner = o.owner;
    
    switch (tt) {
        case symbol_type::VARIABLE:
            variable = o.variable;
            break;
        case symbol_type::OVERLOAD:
            overload = o.overload;
            break;
        case symbol_type::NAMESPACE:
            namespace_ = o.namespace_;
            break;
        case symbol_type::MODULE:
            mod = o.mod;
            break;
        case symbol_type::LABEL:
            label = o.label;
            break;
    }
}

symbol::~symbol() {
    switch (tt) {
        case symbol_type::VARIABLE:
            variable.~symbol_variable();
            break;
        case symbol_type::OVERLOAD:
            overload.~symbol_overload();
            break;
        case symbol_type::NAMESPACE:
            namespace_.~symbol_namespace();
            break;
        case symbol_type::MODULE:
            mod.~symbol_module();
            break;
        case symbol_type::LABEL:
            label.~symbol_label();
            break;
    }
}

symbol::symbol(symbol_type tt, const std::string& name, symbol_table* owner) 
    : tt{tt}, name{name}, owner{owner} {
    
} 

symbol::symbol(const std::string& name, const symbol_variable& variable) 
    : tt{symbol_type::VARIABLE}, name{name}, owner{nullptr}, variable{variable} {
        
}

symbol::symbol(const std::string& name, const symbol_overload& overload) 
    : tt{symbol_type::OVERLOAD}, name{name}, owner{nullptr}, overload{overload} {
    
}

symbol::symbol(const std::string& name, const symbol_namespace& ns) 
    : tt{symbol_type::NAMESPACE}, name{name}, owner{nullptr}, namespace_{ns} {
    
}

symbol::symbol(const std::string& name, const symbol_module& mod) 
    : tt{symbol_type::MODULE}, name{name}, owner{nullptr}, mod{mod} {
    
}

symbol::symbol(const std::string& name, const symbol_label& label) 
    : tt{symbol_type::LABEL}, name{name}, owner{nullptr}, label{label} {
    
}

bool symbol::is_variable() {
    return tt == symbol_type::VARIABLE;
}

bool symbol::is_overload() {
    return tt == symbol_type::OVERLOAD;
}

bool symbol::is_namespace() {
    return tt == symbol_type::NAMESPACE;
}

bool symbol::is_module() {
    return tt == symbol_type::MODULE;
}

bool symbol::is_label() {
    return tt == symbol_type::LABEL;
}

symbol_table::symbol_table(symbol* owner) : owner{owner} {
    
}

symbol_table::~symbol_table() {
    for (auto sym : anonymous) {
        ASSERT(sym, "Anonymous symbol was nullptr");
        delete sym;
    }
    
    for (auto& [name, sym] : symbols) {
        ASSERT(sym, "Symbol was nullptr");
        delete sym;
    }
    
    for (auto st : children) {
        ASSERT(st, "Symbol table was nullptr");
        delete st;
    }
}

symbol_table* symbol_table::make_child(symbol* owner) {
    symbol_table* ret = new symbol_table(owner);
    ret->parent = this;
    children.push_back(ret);
    return ret;
}

symbol* symbol_table::get(const std::string& name) {
    if (auto it = symbols.find(name); it != symbols.end()) {
        return it->second;
    } else if (auto it = borrowed.find(name); it != borrowed.end()) {
        return it->second;
    } else {
        return nullptr;
    }
}

std::pair<symbol_table*, symbol*> symbol_table::find(const std::string& name) {
    if (auto it = symbols.find(name); it != symbols.end()) {
        return {this, it->second};
    } else if (auto it = borrowed.find(name); it != borrowed.end()) {
        return {it->second->owner, it->second};
    } else {
        return parent ? parent->find(name) : std::pair{nullptr, nullptr};
    }
}

symbol* symbol_table::add(const std::string& name, symbol* sym) {
    auto [_, success] = symbols.try_emplace(name, sym);
    if (success) {
        sym->owner = this;
        return sym;
    } else {
        return nullptr;
    }
}

symbol* symbol_table::add_anonymous(symbol* sym) {
    anonymous.push_back(sym);
    sym->owner = this;
    return sym;
}

symbol* symbol_table::add_borrowed(const std::string& name, symbol* sym) {
    auto [_, success] = borrowed.try_emplace(name, sym);
    if (success) {
        return sym;
    } else {
        return nullptr;
    }
}

symbol* symbol_table::add_undefined(const std::string& name, type* t) {
    return add(name, new symbol(name, symbol_variable{t, nullptr, make_child(), false, true, false, false, false, false}));
}

symbol* symbol_table::add_primitive(const std::string& name, type* t, ast* value, bool defined, bool compiletime, bool reference) {
    return add(name, new symbol(name, symbol_variable{t, value, make_child(), defined, compiletime, reference, false, false, false}));
}

symbol* symbol_table::add_type(const std::string& name, type* t, ast* value, symbol_table* st) {
    return add(name, new symbol(name, symbol_variable{t, value, st, true, true, false, false, false, false}));
}

// symbol* symbol_table::add_function(const std::string& name, type* t, ast* value, symbol_table* st) {
//     auto [fst, sym] = find(name);
//     if (sym) {
//         if (fst == this) {
//             if (sym->tt == symbol_type::VARIABLE && sym->variable.t->is_superfunction()) {
//                 symbols.erase(symbols.find(name)); // Wasteful
//                 anonymous.push_back(sym);
//                 
//                 symbol* ol = symbols[name] = new symbol(name, symbol_overload{});
//                 ol->owner = this;
//                 ol->overload.syms.push_back(sym);
//                 symbol* fn = new symbol(name, symbol_variable{t, value, st, true, true, false, false});
//                 fn->owner = this;
//                 ol->overload.syms.push_back(fn);
//                 anonymous.push_back(fn);
//                 
//                 return fn;
//             } else if (sym->tt == symbol_type::OVERLOAD) {
//                 
//                 symbol* fn = new symbol(name, symbol_variable{t, value, st, true, true, false, false});
//                 fn->owner = this;
//                 sym->overload.syms.push_back(fn);
//                 anonymous.push_back(fn);
//                 
//                 return fn;
//             } else {
//                 return nullptr;
//             }
//         
//         } else {
//         
//             if (sym->tt == symbol_type::VARIABLE && sym->variable.t->is_superfunction()) {
//                 
//                 symbol* ol = symbols[name] = new symbol(name, symbol_overload{});
//                 ol->overload.syms.push_back(sym);
//                 symbol* fn = new symbol(name, symbol_variable{t, value, st, true, true, false, false});
//                 fn->owner = this;
//                 ol->overload.syms.push_back(fn);
//                 anonymous.push_back(fn);
//                 
//                 return fn;
//             } else if (sym->tt == symbol_type::OVERLOAD) {
//                 
//                 symbol* ol = symbols[name] = new symbol(*sym);
//                 symbol* fn = new symbol(name, symbol_variable{t, value, st, true, true, false, false});
//                 fn->owner = this;
//                 ol->overload.syms.push_back(fn);
//                 anonymous.push_back(fn);
//                 
//                 return fn;
//             } else {
//                 return nullptr;
//             }
//         }
//     } else {
//         return symbols[name] = new symbol(name, symbol_variable{t, value, st, true, true, false, false});
//     }
// }

symbol* symbol_table::add_namespace(const std::string& name) {
    return add(name, new symbol(name, symbol_namespace{make_child()}));
}

symbol* symbol_table::add_module(const std::string& name, nnmodule* mod) {
    return add(name, new symbol(name, symbol_module{mod}));
}

symbol* symbol_table::add_label(const std::string& name) {
    return add(name, new symbol(name, symbol_label{}));
}

symbol* symbol_table::make_overload(const std::string& name) {
    auto it = symbols.find(name);
    symbol_table* fst{nullptr};
    symbol* sym{nullptr};
    
    if (it != symbols.end()) {
        sym = it->second;
        fst = nullptr;
    } else if (auto it = borrowed.find(name); it != borrowed.end()) {
        sym = it->second;
        fst = sym->owner;
    } else {
        std::tie(fst, sym) = parent ? parent->find(name) : std::pair{nullptr, nullptr};
    }
    
    if (sym) {
        if (fst == this) {
            if (sym->tt == symbol_type::OVERLOAD) {
                return sym;
            } else {
            
                symbols.erase(it);
                anonymous.push_back(sym);
                symbol* ol = symbols[name] = new symbol{name, symbol_overload{}};
                ol->owner = this;
                ol->overload.syms.push_back(sym);
                
                return ol;
            }
        } else {
            if (sym->tt == symbol_type::OVERLOAD) {
                symbol* ol = symbols[name] = new symbol{*sym};
                ol->owner = this;
                
                return ol;
            } else {
                symbol* ol = symbols[name] = new symbol{name, symbol_overload{}};
                ol->owner = this;
                ol->overload.syms.push_back(sym);
                
                return ol;
            }
        }
    } else {
        return nullptr;
    }
}

symbol* symbol_table::borrow(symbol_table* o) {
    symbol* offending {nullptr};
    for (auto& [name, elem] : o->symbols) {
        if(!add_borrowed(name, elem)) {
            offending = offending ? offending : elem;
        }
    }
    
    return offending;
}

symbol* symbol_table::get_owner() {
    return owner;
}
