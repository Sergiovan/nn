#pragma once

#include <string>
#include <variant>
#include <vector>

#include "common/convenience.h"

using param_flags = u8;

struct ast;
struct st_entry;

enum class ettype {
    PRIMITIVE, POINTER, STRUCT, UNION,
    ENUM, COMBINATION, FUNCTION, PSTRUCT, PFUNCTION
};

enum class eptr_type {
    NAKED, ARRAY, SHARED, WEAK, UNIQUE
};

namespace etype_ids {
    constexpr type_id VOID    = 0;
    
    constexpr type_id BYTE    = 1;
    constexpr type_id SHORT   = 2;
    constexpr type_id INT     = 3;
    constexpr type_id LONG    = 4;
    
    constexpr type_id SIG     = 5;
    
    constexpr type_id FLOAT   = 6;
    constexpr type_id DOUBLE  = 7;
    
    constexpr type_id BOOL    = 8;
    
    constexpr type_id CHAR    = 9;
    constexpr type_id STRING  = 10;
    
    constexpr type_id FUN     = 11;
    constexpr type_id LET     = 12;
    
    constexpr type_id NNULL   = 13;
    constexpr type_id NOTHING = 14;
    
    constexpr type_id LAST = 14;
} 

constexpr bool is_integer_type(type_id id) {
    using namespace etype_ids;
    return id == BYTE || id == SHORT || id == INT || id == LONG;
}

constexpr bool is_real_type(type_id id) {
    using namespace etype_ids;
    return id == FLOAT || id == DOUBLE;
}

constexpr bool is_number_type(type_id id) {
    return is_integer_type(id) || is_number_type(id);
}

constexpr bool is_infer_type(type_id id) {
    using namespace etype_ids;
    return id == FUN || id == LET;
}

constexpr bool is_illegal_type(type_id id) {
    using namespace etype_ids;
    return id == VOID || id == NNULL || id == NOTHING;
} 

constexpr bool is_ptr_type(ettype id) {
    return id == ettype::POINTER || id == ettype::FUNCTION;
}

constexpr bool is_pod_type(ettype id) {
    return id == ettype::STRUCT || id == ettype::UNION || id == ettype::ENUM;
}

namespace etype_flags {
    constexpr type_flags SIGNED   = 1 << 0;
    constexpr type_flags CONST    = 1 << 1;
    constexpr type_flags VOLATILE = 1 << 2;
};

namespace eparam_flags {
    constexpr param_flags DEFAULTABLE = 1 << 0;
    constexpr param_flags SPREAD      = 1 << 1;
    constexpr param_flags GENERIC     = 1 << 2;
    // constexpr param_flags GENERIC_AS  = 1 << 3;
};

struct type;

struct pfield {
    type* t;
    u8 bits{64};
    bool bitfield{false};
};

struct field {
    type* t;
    ast* value{nullptr}; // Owned
    u8 bits{64};
    std::string name{""};
    bool bitfield{false};
    
    field() = default;
    ~field();
    
    field(const field& o);
    field(field&& o);
    field& operator=(const field& o);
    field& operator=(field&& o);
};

struct ufield {
    type* t;
    std::string name{""};
};

struct pparameter {
    type* t;
    param_flags flags;
};

struct parameter {
    type* t;
    param_flags flags;
    std::string name{""};
    ast* value{nullptr};
    // std::string type_same_as{""}
};

struct type_primitive {
    type_id t{etype_ids::VOID};
};

struct type_pointer {
    eptr_type ptr_t{eptr_type::NAKED};
    u64 size{0}; // For arrays
    type* t{nullptr};
};

struct type_pstruct {
    std::vector<pfield> fields{};
    u64 size{0};
};

struct type_struct {
    type_pstruct* pure{nullptr};
    std::vector<field> fields{};
    st_entry* ste{nullptr};
};

struct type_union {
    std::vector<ufield> fields{};
    st_entry* ste{nullptr};
    u64 def_type{0};
    ast* def_value{nullptr}; // Owned
    u64 size{0};
    
    type_union(const std::vector<ufield> fields = {}, st_entry* ste = nullptr, u64 def_type = 0,
               ast* def_value = nullptr, u64 size = 0);
    ~type_union();
    
    type_union(const type_union& o);
    type_union(type_union&& o);
    type_union& operator=(const type_union& o);
    type_union& operator=(type_union&& o);
};

struct type_enum {
    st_entry* ste{nullptr};
    u64 size{0};
};

struct type_combination {
    std::vector<type*> types{};
};

struct type_pfunction {
    type* rets{nullptr};
    std::vector<pparameter> params{};
};

struct type_function {
    type_pfunction* pure{nullptr};
    type* rets{nullptr};
    std::vector<parameter> params{};
    st_entry* ste{nullptr};
};

using type_variant = std::variant<type_primitive, type_pointer, 
                                  type_pstruct, type_struct, type_union, 
                                  type_enum, type_combination, type_function, type_pfunction>;

struct type {
    type() = default;
    type(ettype ttype, type_id id = 0, type_flags flags = 0);
    type(ettype ttype, type_id id, type_flags flags, type_variant t);
    type(type_primitive t);
    type(type_pointer t);
    type(type_pstruct t);
    type(type_struct t);
    type(type_union t);
    type(type_enum t);
    type(type_combination t);
    type(type_function t);
    type(type_pfunction t);
    
    ettype     tt;
    type_id    id{0};
    type_flags flags{0};
    type_variant t;
    
    u64 get_size();
    
    bool can_boolean();
    bool can_weak_cast(type* o); // Weak cast this to o
    bool can_cast(type* o); // Cast this to o
    bool is_weak_equalish(type* o); // Can either weak cast to o or be weak cast from o
    bool is_equalish(type* o); // Can either cast to o or be cast from o
    
    
    static type* weak_cast_target(type* a, type* b); // Returns type that can be weak cast to the other
    static type* weak_cast_result(type* a, type* b);
    static type* cast_target(type* a, type* b); // Returns type that can be cast to the other
    static type* cast_result(type* a, type* b);
    
    static type* primitive();
    static type* pointer();
    static type* pstruct();
    static type* _struct();
    static type* _union();
    static type* _enum();
    static type* combination();
    static type* function();
    static type* pfunction();
    
    type_primitive&   as_primitive();
    type_pointer&     as_pointer();
    type_pstruct&     as_pstruct();
    type_struct&      as_struct();
    type_union&       as_union();
    type_enum&        as_enum();
    type_combination& as_combination();
    type_function&    as_function();
    type_pfunction&   as_pfunction();
    
    bool is_primitive(int type = -1);
    bool is_numeric();
    bool is_integer();
    bool is_real();
    bool is_let();
    bool is_fun();
    bool is_pointer();
    bool is_pointer(eptr_type type);
    bool is_struct(bool pure = false);
    bool is_union();
    bool is_enum();
    bool is_combination();
    bool is_function(bool pure = false);
};
