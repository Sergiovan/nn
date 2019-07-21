#include "common/symbol_table.h"

#include <utility>

#include "common/ast.h"
#include "common/type.h"
#include "common/type_table.h"
#include "common/utils.h"

std::string overload::unique_name() {
    using namespace std::string_literals;
    std::stringstream ss{};
    type_function& fun = t->as_function();
    ss << ":";
    if (fun.ste->name.length()) {
        ss << fun.ste->name;
    }
    ss << "(";
    for (auto& param : fun.params) {
        if (param.flags & eparam_flags::SPREAD) {
            ss << param.t->as_pointer().t->print(true);
            ss << "...";
        } else {
            ss << param.t->print(true);
        }
        if (param.name.length()) {
            ss << " " << param.name;
        }
        if (param.flags & eparam_flags::DEFAULTABLE) {
            ss << "=";
        }
        if (&param != &fun.params.back()) {
            ss << ", ";
        }
    }
    ss << ")";
    return ss.str();
}

overload* st_function::get_overload(const std::vector<type*>& args) {
    std::vector<overload*> ret{};
    for (auto& ov : overloads) {
        auto& oparams = ov->t->as_function().params;
        bool spread = oparams.size() ? (oparams.back().flags & eparam_flags::SPREAD) != 0 : false;
        if (args.size() > oparams.size() && !spread) {
            goto next_overload;
        }
        for (u64 i = 0; i < args.size(); ++i) {
            if (i < oparams.size()) {
                if (!args[i]->can_weak_cast(oparams[i].in_param())) {
                    goto next_overload;
                }
            } else if (oparams.back().flags & eparam_flags::SPREAD){
                if (!args[i]->can_weak_cast(oparams.back().in_param())) {
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
        ret.push_back(ov);
        next_overload:;
    }
    if (ret.size() == 1) {
        return ret.front();
    } else if (ret.empty()) {
        return nullptr;
    } else {
        std::vector<overload*> rret{};
        for (auto& ov : ret) {
            auto& oparams = ov->t->as_function().params;
            bool spread = (oparams.back().flags & eparam_flags::SPREAD) != 0;
            for (u64 i = 0; i < oparams.size() - (spread ? 1 : 0); ++i) {
                if (oparams[i].in_param() != args[i]) {
                    goto final_next_overload;
                }
            }
            rret.push_back(ov);
            final_next_overload:;
        }
        if (rret.size() == 1) {
            return ret.front();
        } else {
            return nullptr;
        }
    }
}

st_overload::st_overload(overload* ol, st_function* function) : ol(ol), function(function) { }

st_overload::~st_overload() {
    if (ol) {
        delete ol;
    }
}

st_overload::st_overload(const st_overload& o) {
    if (o.ol) {
        ol = new overload{*o.ol};
    } else {
        ol = nullptr;
    }
    function = o.function;
}

st_overload::st_overload(st_overload&& o) {
    std::swap(ol, o.ol);
    function = o.function;
}

st_overload& st_overload::operator=(const st_overload& o) {
    if (this != &o) {
        if (o.ol) {
            ol = new overload{*o.ol};
        } else {
            ol = nullptr;
        }
        function = o.function;
    }
    
    return *this;
}

st_overload& st_overload::operator=(st_overload&& o) {
    if (this != &o) {
        std::swap(ol, o.ol);
        function = o.function;
    }
    
    return *this;
}

symbol_table::symbol_table(etable_owner owner, symbol_table* parent) : parent(parent), owner(owner) {
    
}

symbol_table::~symbol_table() {
    for (auto& entry : entries) {
        if (entry.second) {
            delete entry.second;
        }
    }
    
    for (auto child : children) {
        if (child) {
            delete child;
        }
    }
}

symbol_table::symbol_table(const symbol_table& o) {
    parent = o.parent;
    owner = o.owner;
    
    for (auto& oentry : o.entries) {
        entries.insert({oentry.first, oentry.second ? new st_entry{*oentry.second} : nullptr});
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
            entries.insert({oentry.first, oentry.second ? new st_entry{*oentry.second} : nullptr});
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

st_entry* st_entry::variable(const std::string& name, type* t, ast* value, bool defined) {
    return new st_entry{st_variable{t, value, defined}, est_entry_type::VARIABLE, name};
}

st_entry * st_entry::field(const std::string& name, u64 field, type* ptype) {
    return new st_entry{st_field{field, ptype}, est_entry_type::FIELD, name};
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

st_overload& st_entry::as_overload() {
    return std::get<st_overload>(entry);
}

st_namespace& st_entry::as_module() {
    return std::get<st_namespace>(entry);
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

bool st_entry::is_overload() {
    return t == est_entry_type::OVERLOAD;
}


bool st_entry::is_label() {
    return t == est_entry_type::LABEL;
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
            return as_function().overloads.size() != 1 ? type_table::t_fun : as_function().overloads.back()->t;
        case est_entry_type::OVERLOAD:
            return as_overload().ol->t;
        case est_entry_type::TYPE:
            return as_type().t;
        case est_entry_type::VARIABLE:
            return as_variable().t;
        case est_entry_type::MODULE: [[fallthrough]];
        case est_entry_type::NAMESPACE: [[fallthrough]];
        case est_entry_type::LABEL: 
            return type_table::t_void;
    }
    return nullptr; // What
}

std::string st_entry::print(u64 depth) {
    std::string sep(depth * 2, ' ');
    std::stringstream ss{};
    ss << sep << std::hex;
    switch (t) {
        case est_entry_type::FIELD:  {
            type* ftype = as_field().ptype;
            ss << "FIELD " << name << " " << (u64) this << " (";
            switch (ftype->tt) {
                case ettype::FUNCTION:
                    ss << type_table::t_sig->print(true);
                    break;
                case ettype::ENUM:
                    ss << ftype->print(true);
                    break;
                case ettype::STRUCT:
                    ss << ftype->as_struct().fields[as_field().field].t->print(true);
                    break;
                case ettype::UNION:
                    ss << ftype->as_union().fields[as_field().field].t->print(true);
                    break;
                default:
                    ss << "NULLPTR"; // Error, bad
                    break;
            }
            ss << ")\n";
            break;
        }
        case est_entry_type::FUNCTION:
            ss << "FUNCTION " << name << " " << (u64) this << "[" << as_function().overloads.size() << "]\n";
            for (auto& ol : as_function().overloads) {
                ss << sep << "  " << ol->t->print(true) << "\n";
                if (ol->defined) {
                    ss << sep << "  " << "Value: \n";
                    ss << (ol->value ? ol->value->print(depth + 2) : "NULLPTR") << "\n";
                    ss << sep << "  " << "Symbol table: \n";
                    ss << (ol->st ? ol->st->print(depth + 2) : "NULLPTR") << "\n";
                }
            }
            as_function().st->print(depth + 2);
            break;
        case est_entry_type::TYPE:
            ss << "TYPE " << name << " " << (u64) this << "\n";
            if (as_type().defined) {
                ss << sep << "  " << as_type().t->print() << "\n";
                ss << as_type().st->print(depth + 1);
            }
            break;
        case est_entry_type::VARIABLE:
            ss << "VARIABLE " << name << " " << (u64) this << "(" << as_variable().t << ")\n";
            if (as_variable().defined) {
                ss << as_variable().value->print(depth + 1);
            }
            break;
        case est_entry_type::MODULE: 
            ss << "MODULE " << name << " " << (u64) this << "\n";
            ss << as_module().st->print(depth + 1);
            break;
        case est_entry_type::NAMESPACE: 
            ss << "NAMESPACE " << name << "\n";
            ss << as_module().st->print(depth + 1);
            break;
        case est_entry_type::OVERLOAD:
            ss << "OVERLOAD " << name << " " << (u64) this;
            if (as_overload().ol->builtin) {
                ss << " BUILTIN(" << as_overload().ol->builtin << ")";
            }
            ss << "\n";
            break;
        case est_entry_type::LABEL: 
            ss << "LABEL " << name << " " << (u64) this << "\n";
            break;;
    }
    return ss.str();
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
        nt.st = make_child(etable_owner::STRUCT);
    }
    st_entry* ne = new st_entry{std::move(nt), est_entry_type::TYPE, name};
    entries.insert({name, ne});
    return ne;
}

st_entry* symbol_table::add_variable(const std::string& name, type* t, ast* value) {
    if (has(name, false)) {
        return nullptr;
    }
    
    st_variable nv{t, value, value != nullptr};
    st_entry* ne = new st_entry{std::move(nv), est_entry_type::VARIABLE, name};
    entries.insert({name, ne});
    return ne;
}

st_entry* symbol_table::add_or_get_empty_function(const std::string& name) {
    st_entry* f = get(name, false);
    if (f && !f->is_function()) {
        return nullptr;
    } else if (!f) {
        f = new st_entry{st_function{{}, make_child(etable_owner::FUNCTION)}, est_entry_type::FUNCTION, name};
        entries.insert({name, f});
    }
    
    return f;
}

std::pair<st_entry*, overload*> symbol_table::add_function(const std::string& name, type* function, ast* value, symbol_table* st) {
    st_entry* f = get(name, false);
    if (f && !f->is_function()) {
        return {nullptr, nullptr};
    } else if (!f) {
        f = new st_entry{st_function{{}, make_child(etable_owner::FUNCTION)}, est_entry_type::FUNCTION, name};
        entries.insert({name, f});
    }
    
    overload* nol = new overload{function, value, value != nullptr, st ? st : make_child(etable_owner::FUNCTION), f->as_function().overloads.size()};
    f->as_function().overloads.push_back(nol);
    
    std::string unique_name = nol->unique_name();
    st_entry* ne = new st_entry{st_overload{nol, &f->as_function()}, est_entry_type::OVERLOAD, unique_name};
    f->as_function().st->entries.insert({unique_name, ne});
    
    return {f, nol};
}

st_entry* symbol_table::add_namespace(const std::string& name, symbol_table* st) {
    if (has(name, false)) {
        return nullptr;
    }
    
    st_namespace nn{st};
    st_entry* ne = new st_entry{std::move(nn), est_entry_type::NAMESPACE, name};
    entries.insert({name, ne});
    return ne;
}

st_entry* symbol_table::add_module(const std::string& name, symbol_table* st) {
    if (has(name, false)) {
        return nullptr;
    }
    
    st_namespace nm{st};
    st_entry* ne = new st_entry{std::move(nm), est_entry_type::MODULE, name};
    entries.insert({name, ne});
    return ne;
}

st_entry* symbol_table::add_field(const std::string& name, u64 field, type* ptype) {
    if (has(name, false)) {
        return nullptr;
    }
    
    st_field nf{field, ptype};
    st_entry* ne = new st_entry{std::move(nf), est_entry_type::FIELD, name};
    entries.insert({name, ne});
    return ne;
}

st_entry* symbol_table::add_label(const std::string& name) {
    using namespace std::string_literals;
    if (has(name, false)) {
        return nullptr;
    }
    
    st_entry* ne = new st_entry{st_label{}, est_entry_type::LABEL, ":"s + name};
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

void symbol_table::set_owner(etable_owner owner) {
    symbol_table::owner = owner;
}

bool symbol_table::owned_by(etable_owner owner) {
    return symbol_table::owner == etable_owner::COPY ? parent->owned_by(owner) : symbol_table::owner == owner;
}

symbol_table* symbol_table::make_child(etable_owner new_owner) {
    symbol_table* child = new symbol_table{new_owner == etable_owner::COPY ? owner : new_owner, this};
    children.push_back(child);
    return child;
}

std::string symbol_table::print(u64 depth) {
    std::string sep(depth * 2, ' ');
    std::stringstream ss{};
    ss << sep;
    
    switch(owner) {
        case etable_owner::BLOCK:
            ss << "Block ";
            break;
        case etable_owner::ENUM:
            ss << "Enum ";
            break;
        case etable_owner::FREE:
            ss << "Top level ";
            break;
        case etable_owner::LOOP:
            ss << "Loop ";
            break;
        case etable_owner::FUNCTION:
            ss << "Function ";
            break;
        case etable_owner::MODULE:
            ss << "Module ";
            break;
        case etable_owner::NAMESPACE:
            ss << "Namespace ";
            break;
        case etable_owner::STRUCT:
            ss << "Struct ";
            break;
        case etable_owner::UNION:
            ss << "Union ";
            break;
        case etable_owner::COPY:
            ss << "Bad ";
            break;
    }
    ss << "symbol table: " << std::hex << (u64) this << "\n";
    ss << sep << "Parent: " << (u64) parent << "\n";
    ss << sep << "Entries: \n";
    if (entries.empty()) {
        ss << sep << "  None\n";
    } else {
        for (auto& entry : entries) {
            ss << entry.second->print(depth + 1);
        }
    }
    ss << sep << "Borrowed entries: \n";
    if (borrowed_entries.empty()) {
        ss << sep << "  None\n";
    } else {
        for (auto& entry : borrowed_entries) {
            ss << sep << "  " << entry.first << "\n";
        }
    }
    return ss.str();
}

std::ostream& operator<<(std::ostream& os, const est_entry_type& st_entry_type) {
    switch (st_entry_type) {
        case est_entry_type::TYPE:
            return os << "TYPE";
        case est_entry_type::VARIABLE:
            return os << "VARIABLE";
        case est_entry_type::FUNCTION:
            return os << "FUNCTION";
        case est_entry_type::NAMESPACE:
            return os << "NAMESPACE";
        case est_entry_type::FIELD:
            return os << "FIELD";
        case est_entry_type::OVERLOAD:
            return os << "OVERLOAD";
        case est_entry_type::MODULE:
            return os << "MODULE";
        case est_entry_type::LABEL:
            return os << "LABEL";
        default:
            return os << "INVALID";
    }
}
