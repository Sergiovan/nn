#pragma once

#include <memory>
#include <variant>
#include <vector>

#include "trie.h"
#include "convenience.h"

enum class SymbolTableEntryType {
    TYPE, VARIABLE, FUNCTION, ALIAS
};

namespace TypeFlag {
    constexpr u8 VOLATILE = 1 << 0;
    constexpr u8 CONST    = 1 << 1;
    constexpr u8 SIGNED   = 1 << 2;
};

struct st_entry_type {
    u8 flags;
    u64 id;
};
struct st_entry_variable {
    u8 flags;
    u64 id;
    //u64 location;
};

struct st_entry_function {
    std::vector<u64> overloads;
};

struct st_entry_alias {
    SymbolTableEntryType type;
    u64 id;
};

using st_entry_union = std::variant<st_entry_type, st_entry_variable, st_entry_function, st_entry_alias>;

struct st_entry {
    st_entry_union entry;
    SymbolTableEntryType type;
    
    st_entry();
};

class symbol_table {
public:
    symbol_table();
    symbol_table(std::nullptr_t);
    symbol_table(std::weak_ptr<symbol_table>&& parent);
    
    bool has(std::string& value) const noexcept;
    st_entry& search(std::string& value) const;
private:
    std::weak_ptr<symbol_table> parent;
    trie<st_entry> values;
};
