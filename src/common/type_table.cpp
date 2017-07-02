#include "type_table.h"

type_struct_union::type_struct_union() {
    elem_names = new symbol_table;
}

type_struct_union::~type_struct_union() {
    delete elem_names;
}

type_enum::type_enum() {
    enum_names = new trie<u64>;
}

type_enum::~type_enum() {
    delete enum_names;
}

type_table::type_table() {

}

u64 type_table::add_type(type& t) {
    types.push_back(t);
    return types.size() - 1;
}