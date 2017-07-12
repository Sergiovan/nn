#include "symbol_table.h"

st_entry::st_entry() : entry(st_entry_variable{0}), type(SymbolTableEntryType::VARIABLE) {}

uid st_entry::get_type() {
    switch(type) {
        case SymbolTableEntryType::TYPE: return std::get<0>(entry).id;
        case SymbolTableEntryType::VARIABLE: return std::get<1>(entry).id;
        case SymbolTableEntryType::FUNCTION: return TypeID::FUN;
    }
}

symbol_table::symbol_table() : parent() {}

symbol_table::symbol_table(std::nullptr_t) : parent() {}

symbol_table::symbol_table(std::weak_ptr<symbol_table>&& parent) : parent(parent) {}

bool symbol_table::has(std::string& value) const noexcept {
    return values.get(value.c_str()).has_value();
}

st_entry* symbol_table::search(std::string& value) const {
    auto ret = values.get(value.c_str());
    return *ret;
}

st_entry* symbol_table::search(std::string&& value) const {
    auto ret = values.get(value.c_str());
    return *ret;
}