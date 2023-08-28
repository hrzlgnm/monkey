#include <algorithm>
#include <string>

#include "object.hpp"

#include <ast/builtin_function_expression.hpp>
#include <ast/callable_expression.hpp>
#include <fmt/format.h>

auto operator<<(std::ostream& ostrm, object::object_type type) -> std::ostream&
{
    using enum object::object_type;
    switch (type) {
        case integer:
            return ostrm << "integer";
        case boolean:
            return ostrm << "boolean";
        case string:
            return ostrm << "string";
        case null:
            return ostrm << "null";
        case error:
            return ostrm << "error";
        case array:
            return ostrm << "array";
        case hash:
            return ostrm << "hash";
        case function:
            return ostrm << "function";
        case compiled_function:
            return ostrm << "compiled_function";
        case closure:
            return ostrm << "closure";
        case builtin:
            return ostrm << "builtin";
        default:
            return ostrm << "unknown " << static_cast<int>(type);
    }
}

auto string_object::hash_key() const -> hash_key_type
{
    return value;
}

auto boolean_object::hash_key() const -> hash_key_type
{
    return value;
}

auto integer_object::hash_key() const -> hash_key_type
{
    return value;
}

auto builtin_object::inspect() const -> std::string
{
    return fmt::format("builtin {}(){{...}}", builtin->name);
}
