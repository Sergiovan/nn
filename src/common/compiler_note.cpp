#include "compiler_note.h"

using namespace CompilerNote;

note note::empty() {
    return {enote_type::INVALID, note_empty{}};
}

note note::builtin(u64 value) {
    return {enote_type::BUILTIN, note_builtin{value}};
}

note_builtin& note::as_builtin() {
    return std::get<note_builtin>(v);
}

bool note::is_builtin() {
    return t == enote_type::BUILTIN;
}

bool note::is_invalid() {
    return t == enote_type::INVALID;
}

std::ostream& operator<<(std::ostream& os, enote_type t) {
    if (auto it = note_names.find(t); it != note_names.end()) {
        return os << it->second;
    } else {
        return os << "INVALID";
    }
}
