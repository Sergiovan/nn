#pragma once

#include <string>
#include "common/defs.h"
#include "common/type.h"

class type_table {
public:
    type* add_primitive(const type_primitive& p, const bool _const, const bool volat);
    type* add_pointer(const type_pointer& p, const bool _const, const bool volat);
    type* add_array(const type_array& a, const bool _const, const bool volat);
    type* add_compound(const type_compound& c, const bool _const, const bool volat);
    type* add_supercompound(const type_supercompound& sc, type_type sct, const bool _const, const bool volat);
    type* add_function(const type_function& f, const bool _const, const bool volat);
    type* add_superfunction(const type_superfunction& sf, const bool _const, const bool volat);
    type* add_special(const type_special& s, const bool _const, const bool volat);
    
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
private:
    type* add(type* t);
    
    std::string mangle(type* t);
    type unmangle(const std::string& s);
    type* get(type* t);
    type* get_or_add(type* t);
    
    std::vector<type*> types{};
    dict<std::string, type*> mangle_table{};
};
