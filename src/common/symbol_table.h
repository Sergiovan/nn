#pragma once

#include <memory>
#include <variant>
#include <vector>

#include "convenience.h"
#include "trie.h"
#include "type.h"

enum class SymbolTableEntryType {
    TYPE, VARIABLE, FUNCTION
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
    value data;
    vflags flags;
    bool defined;
};

struct st_entry_function {
    std::vector<overload> overloads;
};


using st_entry_union = std::variant<st_entry_type, st_entry_variable, st_entry_function>;

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
    st_entry* search(std::string& value) const;
private:
    std::weak_ptr<symbol_table> parent;
    trie<st_entry*> values;
};
