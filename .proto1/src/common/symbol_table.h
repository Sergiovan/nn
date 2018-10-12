#pragma once

#include <memory>
#include <variant>
#include <vector>

#include "convenience.h"
#include "type.h"

class symbol_table;

enum class SymbolTableEntryType {
    TYPE, VARIABLE, FUNCTION, NAMESPACE
};

namespace TypeFlag {
    constexpr u8 VOLATILE = 1 << 0;
    constexpr u8 CONST    = 1 << 1;
    constexpr u8 SIGNED   = 1 << 2;
};

struct st_entry_type {
    uid id;
    bool defined;
};
struct st_entry_variable {
    uid id;
    value data; // TODO It's a pointer
    vflags flags;
    bool defined;
};

struct st_entry_function {
    std::vector<overload> overloads;
    
    st_entry_function(overload o);
};

struct st_entry_namespace {
    symbol_table* st;

    ~st_entry_namespace();
};

using st_entry_union = std::variant<st_entry_type, st_entry_variable, st_entry_function, st_entry_namespace>;

struct st_entry {
    st_entry_union entry;
    SymbolTableEntryType type;
    
    st_entry();
    st_entry(st_entry_union value, SymbolTableEntryType type);
    
    uid get_type();
    st_entry_variable& get_variable();
};

class symbol_table {
public:
    symbol_table();
    symbol_table(std::nullptr_t);
    symbol_table(symbol_table* parent);
    
    bool has(std::string& name) const noexcept;
    st_entry* search(const std::string& name) const;
    st_entry* search(std::string&& name) const;
    
    st_entry* add_type(std::string& name, uid id, bool defined = true);
    st_entry* add_variable(std::string& name, uid type, value data = {nullptr}, vflags flags = 0, bool defined = false);
    st_entry* add_function(std::string& name, overload o);
    st_entry* add_namespace(std::string& name, symbol_table* st);
private:
    symbol_table* parent;
    trie<st_entry*> values;
};
