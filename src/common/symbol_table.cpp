#include "common/symbol_table.h"
#include "type_table.h"

#include "common/ast.h"
#include "common/type.h"
#include "common/utils.h"

overload::overload(type* t, ast* value, bool defined, symbol_table* st) 
 : t(t), value(value), defined(defined), st(st) {
    
}


overload::~overload() {
    if (value) {
        delete value;
    }
    if (st) {
        delete st;
    }
}

overload::overload(const overload& o) {
    if (value) {
        delete value;
    }
    if (st) {
        delete st;
    }
    
    t = o.t;
    if (o.value) {
        value = new ast{*o.value};
    } else {
        value = nullptr;
    }
    defined = o.defined;
    if (o.st) {
        st = new symbol_table{*o.st};
    } else {
        st = nullptr;
    }
}

overload::overload(overload&& o) {
    t = o.t;
    std::swap(value, o.value);
    defined = o.defined;
    std::swap(st, o.st);
}

overload& overload::operator=(const overload& o) {
    if (this != &o) {
        if (value) {
            delete value;
        }
        if (st) {
            delete st;
        }
        
        t = o.t;
        if (o.value) {
            value = new ast{*o.value};
        } else {
            value = nullptr;
        }
        defined = o.defined;
        if (o.st) {
            st = new symbol_table{*o.st};
        } else {
            st = nullptr;
        }
    }
    return *this;
}

overload& overload::operator=(overload&& o) {
    if (this != &o) {
        t = o.t;
        std::swap(value, o.value);
        defined = o.defined;
        std::swap(st, o.st);
    }
    return *this;
}

st_type::st_type(type* t, bool defined, symbol_table* st) :t(t), defined(defined), st(st) {
    
}

st_type::~st_type() {
    if (st) {
        delete st;
    }
}

st_type::st_type(const st_type& o) {
    if (st) {
        delete st;
    }
    
    t = o.t;
    defined = o.defined;
    if (o.st) {
        st = new symbol_table{*o.st};
    } else {
        st = nullptr;
    }
}

st_type::st_type(st_type&& o) {
    t = o.t;
    defined = o.defined;
    std::swap(st, o.st);
}

st_type& st_type::operator=(const st_type& o) {
    if (this != &o) {
        if (st) {
            delete st;
        }
        
        t = o.t;
        defined = o.defined;
        if (o.st) {
            st = new symbol_table{*o.st};
        } else {
            st = nullptr;
        }
    }
    return *this;
}

st_type& st_type::operator=(st_type&& o) {
    if (this != &o) {
        t = o.t;
        defined = o.defined;
        std::swap(st, o.st);
    }
    return *this;
}

st_variable::st_variable(type* t, ast* value, bool defined) : t(t), value(value), defined(defined) {

}

st_variable::~st_variable() {
    if (value) {
        delete value;
    }
}

st_variable::st_variable(const st_variable& o) {
    if (value) {
        delete value;
    }
    
    t = o.t;
    if (o.value) {
        value = new ast{*o.value};
    } else {
        value = nullptr;
    }
    defined = o.defined;
}

st_variable::st_variable(st_variable&& o) {
    t = o.t;
    std::swap(value, o.value);
    defined = o.defined;
}

st_variable& st_variable::operator=(const st_variable& o) {
    if (this != &o) {
        if (value) {
            delete value;
        }
        
        t = o.t;
        if (o.value) {
            value = new ast{*o.value};
        } else {
            value = nullptr;
        }
        defined = o.defined;
    }
    return *this;
}

st_variable& st_variable::operator=(st_variable&& o) {
    if (this != &o) {
        t = o.t;
        std::swap(value, o.value);
        defined = o.defined;
    }
    return *this;
}

st_function::st_function(const std::vector<overload> overloads, symbol_table* st) : overloads(overloads), st(st) {

}

overload* st_function::get_overload(const std::vector<type*>& args) {
    std::vector<overload*> ret{};
    for (auto& ov : overloads) {
        auto& oparams = ov.t->as_function().params;
        bool spread = (oparams.back().flags & eparam_flags::SPREAD) == 0;
        if (args.size() > oparams.size() && !spread) {
            goto next_overload;
        }
        for (u64 i = 0; i < args.size(); ++i) {
            if (i < oparams.size()) {
                if (!args[i]->can_weak_cast(oparams[i].t)) {
                    goto next_overload;
                }
            } else if (oparams.back().flags & eparam_flags::SPREAD){
                if (!args[i]->can_weak_cast(oparams.back().t)) {
                    goto next_overload;
                }
            } else {
                goto next_overload;
            }
        }
        if (args.size() < oparams.size()) {
            for (u64 i = args.size(); i < oparams.size(); ++i) {
                if ((oparams[i].flags & eparam_flags::DEFAULTABLE) == 0) {
                    goto next_overload;
                }
            }
        }
        ret.push_back(&ov);
        next_overload:;
    }
    if (ret.size() == 1) {
        return ret.front();
    } else if (ret.empty()) {
        return nullptr;
    } else {
        // TODO Sort out the rest of the overloads
        return nullptr;
    }
}

st_function::~st_function() {
    if (st) {
        delete st;
    }
}

st_function::st_function(const st_function& o) {
    if (st) {
        delete st;
    }
    
    overloads = o.overloads;
    if (o.st) {
        st = new symbol_table{*o.st};
    } else {
        st = nullptr;
    }
}

st_function::st_function(st_function&& o) {
    std::swap(overloads, o.overloads);
    std::swap(st, o.st);
}

st_function& st_function::operator=(const st_function& o) {
    if (this != &o) {
        if (st) {
            delete st;
        }
        
        overloads = o.overloads;
        if (o.st) {
            st = new symbol_table{*o.st};
        } else {
            st = nullptr;
        }
    }
    return *this;
}

st_function& st_function::operator=(st_function&& o) {
    if (this != &o) {
        std::swap(overloads, o.overloads);
        std::swap(st, o.st);
    }
    return *this;
}

st_namespace::st_namespace(symbol_table* st) : st(st) {

}

st_namespace::~st_namespace() {
    if (st) {
        delete st;
    }
}

st_namespace::st_namespace(const st_namespace& o) {
    if (st) {
        delete st;
    }

    if (o.st) {
        st = new symbol_table{*o.st};
    } else {
        st = nullptr;
    }
}

st_namespace::st_namespace(st_namespace&& o) {
    std::swap(st, o.st);
}

st_namespace& st_namespace::operator=(const st_namespace& o) {
    if (this != &o) {
        if (st) {
            delete st;
        }
        
        if (o.st) {
            st = new symbol_table{*o.st};
        } else {
            st = nullptr;
        }
    }
    return *this;
}

st_namespace& st_namespace::operator=(st_namespace&& o) {
    if (this != &o) {
        std::swap(st, o.st);
    }
    return *this;
}

symbol_table::symbol_table(etable_owner owner, symbol_table* parent) : parent(parent), owner(owner) {
    
}

symbol_table::~symbol_table() {
    for (auto& entry : entries) {
        delete entry.second;
    }
}

symbol_table::symbol_table(const symbol_table& o) {
    for (auto& entry : entries) {
        delete entry.second;
    }

    parent = o.parent;
    owner = o.owner;
    
    for (auto& oentry : o.entries) {
        entries.insert({oentry.first, new st_entry{*oentry.second}});
    }
    borrowed_entries = o.borrowed_entries;
}

symbol_table::symbol_table(symbol_table&& o) {
    parent = o.parent;
    owner = o.owner;
    std::swap(entries, o.entries);
    borrowed_entries = o.borrowed_entries;
}

symbol_table& symbol_table::operator=(const symbol_table& o) {
    if (this != &o) {
        for (auto& entry : entries) {
            delete entry.second;
        }

        parent = o.parent;
        owner = o.owner;
        
        for (auto& oentry : o.entries) {
            entries.insert({oentry.first, new st_entry{*oentry.second}});
        }
        borrowed_entries = o.borrowed_entries;
    }
    return *this;
}

symbol_table& symbol_table::operator=(symbol_table&& o) {
    if (this != &o) {
        parent = o.parent;
        owner = o.owner;
        std::swap(entries, o.entries);
        borrowed_entries = o.borrowed_entries;
    }
    return *this;
}

st_entry* st_entry::variable(type* t, ast* value, bool defined) {
    return new st_entry{st_variable{t, value, defined}, est_entry_type::VARIABLE};
}

st_entry * st_entry::field(u64 field, type* ptype) {
    return new st_entry{st_field{field, ptype}, est_entry_type::FIELD};
}


st_type& st_entry::as_type() {
    return std::get<st_type>(entry);
}

st_variable& st_entry::as_variable() {
    return std::get<st_variable>(entry);
}

st_function& st_entry::as_function() {
    return std::get<st_function>(entry);
}

st_namespace& st_entry::as_namespace() {
    return std::get<st_namespace>(entry);
}

st_field& st_entry::as_field() {
    return std::get<st_field>(entry);
}

bool st_entry::is_type() {
    return t == est_entry_type::TYPE;
}

bool st_entry::is_variable() {
    return t == est_entry_type::VARIABLE;
}

bool st_entry::is_function() {
    return t == est_entry_type::FUNCTION;
}

bool st_entry::is_namespace() {
    return t == est_entry_type::NAMESPACE;
}

bool st_entry::is_field() {
    return t == est_entry_type::FIELD;
}

bool st_entry::is_module() {
    return t == est_entry_type::MODULE;
}

type* st_entry::get_type() {
    switch (t) {
        case est_entry_type::FIELD: 
        {
            type* ftype = as_field().ptype;
            switch (ftype->tt) {
                case ettype::FUNCTION:
                    return type_table::t_sig;
                case ettype::ENUM:
                    return ftype;
                case ettype::STRUCT:
                    return ftype->as_struct().fields[as_field().field].t;
                case ettype::UNION:
                    return ftype->as_union().fields[as_field().field].t;
                default:
                    return nullptr; // Error, bad
            }
        }
        case est_entry_type::FUNCTION:
            return as_function().overloads.size() != 1 ? type_table::t_fun : as_function().overloads[0].t;
        case est_entry_type::TYPE:
            return as_type().t;
        case est_entry_type::VARIABLE:
            return as_variable().t;
        case est_entry_type::MODULE: [[fallthrough]];
        case est_entry_type::NAMESPACE:
            return type_table::t_void;
    }
    return nullptr; // What
}

bool symbol_table::has(const std::string& name, bool propagate, etable_owner until) {
    return get(name, propagate, until) != nullptr;
}

st_entry* symbol_table::get(const std::string& name, bool propagate, etable_owner until) {
    auto entry = entries.find(name);
    if (entry == entries.end()) {
        entry = borrowed_entries.find(name);
    }
    if (entry != entries.end() && entry != borrowed_entries.end()) {
        return entry->second;
    } else if (!propagate || owner == until || !parent) {
        return nullptr;
    } else {
        return parent->get(name, propagate, until);
    }
}

st_entry* symbol_table::get(const std::string& name, est_entry_type t, bool propagate, etable_owner until) {
    auto entry = get(name, propagate, until);
    return entry && entry->t == t ? entry : nullptr;
}

overload* symbol_table::get_overload(const std::string& name, type* ftype, bool propagate, etable_owner until) {
    std::vector<type*> params{};
    auto& fparams = ftype->as_function().params;
    for (auto& param : fparams) {
        params.push_back(param.t);
    }
    return get_overload(name, params, propagate, until);
}

overload* symbol_table::get_overload(const std::string& name, std::vector<type*>& args, bool propagate, etable_owner until) {
    auto entry = get(name);
    if (entry && entry->t == est_entry_type::FUNCTION) {
        return entry->as_function().get_overload(args);
    } else if (!propagate || owner == until || !parent) {
        return nullptr;
    } else {
        return parent->get_overload(name, args, propagate, until);
    }
}

st_entry* symbol_table::add(const std::string& name, st_entry* entry) {
    if (has(name, false)) {
        return nullptr;
    } else {
        entry->name = name;
        entries.insert({name, entry});
        return entry;
    }
}

st_entry* symbol_table::add_type(const std::string& name, type* t, bool defined) {
    if (has(name, false)) {
        return nullptr;
    }
    
    st_type nt{t, defined};
    if (t->is_struct() || t->is_union()) {
        nt.st = new symbol_table(etable_owner::STRUCT, this);
    }
    st_entry* ne = new st_entry{nt, est_entry_type::TYPE, name};
    entries.insert({name, ne});
    return ne;
}

st_entry* symbol_table::add_variable(const std::string& name, type* t, ast* value) {
    if (has(name, false)) {
        return nullptr;
    }
    
    st_variable nv{t, value, value != nullptr};
    st_entry* ne = new st_entry{nv, est_entry_type::VARIABLE, name};
    entries.insert({name, ne});
    return ne;
}

st_entry* symbol_table::add_or_get_empty_function(const std::string& name) {
    st_entry* f = get(name, false);
    if (f && !f->is_function()) {
        return nullptr;
    } else if (!f) {
        f = new st_entry{st_function{}, est_entry_type::FUNCTION, name};
        entries.insert({name, f});
    }
    
    return f;
}

std::pair<st_entry*, overload*> symbol_table::add_function(const std::string& name, type* function, ast* value, symbol_table* st) {
    st_entry* f = get(name, false);
    if (f && !f->is_function()) {
        return {nullptr, nullptr};
    } else if (!f) {
        f = new st_entry{st_function{}, est_entry_type::FUNCTION, name};
        entries.insert({name, f});
    }
    
    overload o{function, value, value != nullptr, st ? st : new symbol_table{etable_owner::FUNCTION, this}};
    f->as_function().overloads.emplace_back(std::move(o));
    return {f, &f->as_function().overloads.back()};
}

st_entry* symbol_table::add_namespace(const std::string& name, symbol_table* st) {
    if (has(name, false)) {
        return nullptr;
    }
    
    st_namespace nn{st};
    st_entry* ne = new st_entry{nn, est_entry_type::NAMESPACE, name};
    entries.insert({name, ne});
    return ne;
}

st_entry* symbol_table::add_module(const std::string& name, symbol_table* st) {
    if (has(name, false)) {
        return nullptr;
    }
    
    st_namespace nm{st};
    st_entry* ne = new st_entry{nm, est_entry_type::MODULE, name};
    entries.insert({name, ne});
    return ne;
}

st_entry* symbol_table::add_field(const std::string& name, u64 field, type* ptype) {
    if (has(name, false)) {
        return nullptr;
    }
    
    st_field nf{field, ptype};
    st_entry* ne = new st_entry{nf, est_entry_type::FIELD, name};
    entries.insert({name, ne});
    return ne;
}

st_entry* symbol_table::borrow(const std::string& name, st_entry* entry) {
    borrowed_entries.insert({name, entry});
    return entry;
}

bool symbol_table::merge_st(symbol_table* st) {
    for (auto& [name, entry] : st->entries) {
        if (has(name, false)) {
            return false;
        } else {
            borrowed_entries.insert({name, entry});
        }
    }
    return true;
}

u64 symbol_table::get_size(bool borrowed) {
    return entries.size() + (borrowed ? borrowed_entries.size() : 0);
}

symbol_table* symbol_table::make_child(etable_owner new_owner) {
    return new symbol_table(new_owner == etable_owner::COPY ? owner : new_owner, this);
}

