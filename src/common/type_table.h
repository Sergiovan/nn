#pragma once

#include <string>
#include "common/defs.h"
#include "common/type.h"

class type_table {
public:
    type_table();
    
    type* add_primitive(const type_primitive& p, const bool _const, const bool volat);
    type* add_pointer(const type_pointer& p, const bool _const, const bool volat);
    type* add_array(const type_array& a, const bool _const, const bool volat);
    type* add_compound(const type_compound& c, const bool _const, const bool volat);
    type* add_supercompound(const type_supercompound& sc, type_type sct, const bool _const, const bool volat);
    type* add_function(const type_function& f, const bool _const, const bool volat);
    type* add_superfunction(const type_superfunction& sf, const bool _const, const bool volat);
    type* add_special(const type_special& s, const bool _const, const bool volat);
    
    type* add_temp(type& t);
    type* update_temp(type* t);
    
    type* get(u64 id) const;
    type* operator[](u64 id) const;
    
    type* get(const type_primitive& p, const bool _const, const bool volat);
    type* get(const type_pointer& p, const bool _const, const bool volat);
    type* get(const type_array& a, const bool _const, const bool volat);
    type* get(const type_compound& c, const bool _const, const bool volat);
    type* get(const type_supercompound& sc, type_type sct, const bool _const, const bool volat);
    type* get(const type_function& f, const bool _const, const bool volat);
    type* get(const type_superfunction& sf, const bool _const, const bool volat);
    type* get(const type_special& s, const bool _const, const bool volat);
    
    type* reflag(type* t, const bool _const, const bool volat);
    type* pointer_to(type* t, pointer_type pt = pointer_type::NAKED, bool _const = false, bool volat = false);
    type* array_of(type* t, bool _const = false, bool volat = false);
    type* sized_array_of(type* t, u64 size, bool _const = false, bool volat = false);
    
    bool can_convert_strong(type* from, type* to);
    bool can_convert_weak(type* from, type* to);
    
    type* propagate_generic(type* t);
    type* get_signed(u16 bits);
    type* get_unsigned(u16 bits);
    
    type* U0;
    type* U1;
    type* U8;
    type* U16;
    type* U32;
    type* U64;
    type* S8;
    type* S16;
    type* S32;
    type* S64;
    type* F32;
    type* F64;
    type* C8;
    type* C16;
    type* C32;
    type* E64;
    type* TYPE;
    type* ANY;
    type* INFER;
    type* NOTHING;
    type* TYPELESS;
    type* NONE;
    type* NONE_ARRAY;
    type* NONE_STRUCT;
    type* NONE_TUPLE;
    type* NONE_FUNCTION;
    type* NULL_;
    type* GENERIC_UNKNOWN;
    type* GENERIC_COMPOUND;
    type* ERROR_TYPE;
    type* ERROR_COMPOUND;
private:
    type* add(type* t);
    
    // TODO Could this cause issues with supercompounds and superfunctions
    // that point to the same thing?
    std::string mangle(type* t);
    type unmangle(const std::string& s);
    type* get(type* t);
    type* get_or_add(type* t);
    
    std::vector<type*> types{};
    dict<std::string, type*> mangle_table{};
};
