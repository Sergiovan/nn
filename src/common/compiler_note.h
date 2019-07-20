#pragma once

#include <map>
#include <string>
#include <iostream>
#include <variant>

#include "common/convenience.h"

namespace CompilerNote {
    
enum class enote_type {
    BUILTIN, INVALID
};
    
struct note_empty{ };

struct note_builtin {
    u64 value{0};
};

using note_variant = std::variant<note_empty, note_builtin>;
    
struct note {
    enote_type t;
    note_variant v;
    
    static note empty();
    static note builtin(u64 value = 0);
    
    note_builtin& as_builtin();
    
    bool is_builtin();
    bool is_invalid();
};

static const dict<enote_type> string_to_note {
    {"builtin", enote_type::BUILTIN}
};

static const std::map<enote_type, std::string> note_names{swap_key(string_to_note)};

}

std::ostream& operator<<(std::ostream& os, CompilerNote::enote_type t);
