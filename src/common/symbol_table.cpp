#include "symbol_table.h"

st_entry_function::st_entry_function(overload o) {
    overloads.push_back(o);
}

st_entry_namespace::~st_entry_namespace() {
    delete st;
};

st_entry::st_entry() : entry(st_entry_variable{0}), type(SymbolTableEntryType::VARIABLE) {}

st_entry::st_entry(st_entry_union value, SymbolTableEntryType type) : entry(value), type(type) {}

uid st_entry::get_type() {
    switch(type) {
        case SymbolTableEntryType::TYPE: return std::get<0>(entry).id;
        case SymbolTableEntryType::VARIABLE: return std::get<1>(entry).id;
        case SymbolTableEntryType::FUNCTION: return TypeID::FUN;
        case SymbolTableEntryType::NAMESPACE: return -1; //TODO Throw
    }
}

symbol_table::symbol_table() : parent() {}

symbol_table::symbol_table(std::nullptr_t) : parent() {}

symbol_table::symbol_table(symbol_table* parent) : parent(parent) {}

bool symbol_table::has(std::string& name) const noexcept {
    return values.find(name.c_str()) != values.end();
}

st_entry* symbol_table::search(const std::string& name) const {
    auto ret = values.find(name.c_str());
    return (ret != values.end() ? ret->second : nullptr);
}

st_entry* symbol_table::search(std::string&& name) const {
    auto ret = values.find(name.c_str());
    return (ret != values.end() ? ret->second : nullptr);
}

st_entry* symbol_table::add_type(std::string& name, uid id, bool defined) {
    if(has(name)) {
        // TODO Throw
    }
    st_entry* entry = new st_entry{st_entry_type{id, defined}, SymbolTableEntryType::TYPE};
    values[name] = entry;
    return entry;
}

st_entry* symbol_table::add_variable(std::string &name, uid type, value data, vflags flags, bool defined) {
    if(has(name)) {
        // TODO Throw
    }
    st_entry* entry = new st_entry{st_entry_variable{type, data, flags, defined}, SymbolTableEntryType::VARIABLE};
    values[name] = entry;
    return entry;
}

st_entry* symbol_table::add_function(std::string &name, overload o) {
    if(has(name)) {
        // TODO Throw
    }
    st_entry* entry = new st_entry{st_entry_function{o}, SymbolTableEntryType::FUNCTION};
    values[name] = entry;
    return entry;
}

st_entry* symbol_table::add_namespace(std::string& name, symbol_table* st) {
    if(has(name)) {
        // TODO Throw
    }
    st_entry* entry = new st_entry{st_entry_namespace{new symbol_table(this)}, SymbolTableEntryType::NAMESPACE};
    values[name] = entry;
    return entry;
}