#include "type.h"

type_enum::type_enum() {
    enum_names = new trie<u16>;
}

type_enum::~type_enum() {
    delete enum_names;
}