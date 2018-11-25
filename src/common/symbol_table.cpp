#include "common/symbol_table.h"

#include "common/ast.h"
#include "common/type.h"


overload::~overload() {
    if (st) {
        delete st;
    }
    if (value) {
        delete value;
    }
}

st_type::~st_type() {
    if (st) {
        delete st;
    }
}

st_variable::~st_variable() {
    if (value) {
        delete value;
    }
}

st_function::~st_function() {
    if (st) {
        delete st;
    }
}

st_namespace::~st_namespace() {
    if (st) {
        delete st;
    }
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

symbol_table::symbol_table(etable_owner owner, symbol_table* parent) : parent(parent), owner(owner) {
    
}

symbol_table::~symbol_table() {
    for (auto& entry : entries) {
        delete entry.second;
    }
}

bool symbol_table::has(const std::string& name, bool propagate, etable_owner until) {
    return get(name, propagate, until) != nullptr;
}

st_entry* symbol_table::get(const std::string& name, bool propagate, etable_owner until) {
    auto entry = entries.find(name);
    if (entry == entries.end()) {
        entry = borrowed_entries.find(name);
    }
    if (entry != entries.end()) {
        return entry->second;
    } else if (!propagate || owner == until || !parent) {
        return nullptr;
    } else {
        return parent->get(name, propagate, until);
    }
}

st_entry* symbol_table::get(const std::string& name, est_entry_type t, bool propagate, etable_owner until) {
    auto entry = get(name, propagate, until);
    return entry->t == t ? entry : nullptr;
}

overload* symbol_table::get_overload(const std::string& name, type* ftype, bool propagate, etable_owner until) {
    std::vector<type*> params{};
    auto& fparams = ftype->as_function().params;
    for (auto& param : fparams) {
        params.push_back(param.t);
    }
    return get_overload(name, params, propagate, until);
}

overload* symbol_table::get_overload(const std::string& name, std::vector<type*>& params, bool propagate, etable_owner until) {
    auto entry = get(name);
    if (entry && entry->t == est_entry_type::FUNCTION) {
        std::vector<overload>& overloads = entry->as_function().overloads;
        std::vector<overload*> ret{};
        for (auto& ov : overloads) {
            auto& oparams = ov.t->as_function().params;
            bool spread = (oparams.back().flags & eparam_flags::SPREAD) == 0;
            if (params.size() > oparams.size() && !spread) {
                goto next_overload;
            }
            for (u64 i = 0; i < params.size(); ++i) {
                if (i < oparams.size()) {
                    if (!params[i]->can_weak_cast(oparams[i].t)) {
                        goto next_overload;
                    }
                } else {
                    if (!params[i]->can_weak_cast(oparams.back().t)) {
                        goto next_overload;
                    }
                }
            }
            if (params.size() < oparams.size()) {
                for (u64 i = params.size(); i < oparams.size(); ++i) {
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
    } else if (!propagate || owner == until || !parent) {
        return nullptr;
    } else {
        return parent->get_overload(name, params, propagate, until);
    }
}

st_entry* symbol_table::add(const std::string& name, st_entry* entry) {
    if (has(name, false)) {
        return nullptr;
    } else {
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
    st_entry* ne = new st_entry{nt, est_entry_type::TYPE};
    entries.insert({name, ne});
    return ne;
}

st_entry* symbol_table::add_variable(const std::string& name, type* t, ast* value) {
    if (has(name, false)) {
        return nullptr;
    }
    
    st_variable nv{t, value, value != nullptr};
    st_entry* ne = new st_entry{nv, est_entry_type::VARIABLE};
    entries.insert({name, ne});
    return ne;
}

st_entry* symbol_table::add_or_get_empty_function(const std::string& name) {
    st_entry* f = get(name, false);
    if (f && !f->is_function()) {
        return nullptr;
    } else if (!f) {
        f = new st_entry{st_function{}, est_entry_type::FUNCTION};
        entries.insert({name, f});
    }
    
    return f;
}

std::pair<st_entry*, overload*> symbol_table::add_function(const std::string& name, type* function, ast* value, symbol_table* st) {
    st_entry* f = get(name, false);
    if (f && !f->is_function()) {
        return {nullptr, nullptr};
    } else if (!f) {
        f = new st_entry{st_function{}, est_entry_type::FUNCTION};
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
    st_entry* ne = new st_entry{nn, est_entry_type::NAMESPACE};
    entries.insert({name, ne});
    return ne;
}

st_entry* symbol_table::add_module(const std::string& name, symbol_table* st) {
    if (has(name, false)) {
        return nullptr;
    }
    
    st_namespace nm{st};
    st_entry* ne = new st_entry{nm, est_entry_type::MODULE};
    entries.insert({name, ne});
    return ne;
}

st_entry* symbol_table::add_field(const std::string& name, u64 field) {
    if (has(name, false)) {
        return nullptr;
    }
    
    st_field nf{field};
    st_entry* ne = new st_entry{nf, est_entry_type::FIELD};
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

