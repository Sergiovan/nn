#include "type.h"

type_primitive &type::get_primitive() {
    return std::get<0>(data);
}

type_ptr &type::get_ptr() {
    return std::get<1>(data);
}

type_struct &type::get_struct() {
    return std::get<2>(data);
}

type_union &type::get_union() {
    return std::get<3>(data);
}

type_enum &type::get_enum() {
    return std::get<4>(data);
}

type_func &type::get_func() {
    return std::get<5>(data);
}