#include <sstream>
#include <string>

#include "object.hpp"

#include <ast/builtin_function_expression.hpp>
#include <ast/callable_expression.hpp>
#include <fmt/format.h>
#include <fmt/ostream.h>

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

auto function_object::inspect() const -> std::string
{
    return callable->string();
}

auto array_object::inspect() const -> std::string
{
    std::stringstream str;
    str << "[";
    for (bool first = true; const auto* const element : elements) {
        if (!first) {
            str << ", ";
        }
        str << fmt::format("{}", element->inspect());
        first = false;
    }
    str << "]";
    return str.str();
}

namespace
{
auto operator<<(std::ostream& strm, const hashable_object::hash_key_type& t) -> std::ostream&
{
    std::visit(
        overloaded {
            [&](int64_t val) { strm << val; },
            [&](const std::string& val) { strm << '"' << val << '"'; },
            [&](bool val) { strm << val; },
        },
        t);
    return strm;
}

}  // namespace

auto hash_object::inspect() const -> std::string
{
    std::stringstream str;
    str << "{";
    for (bool first = true; const auto& [key, value] : pairs) {
        if (!first) {
            str << ", ";
        }
        str << key;
        str << ": " << value->inspect();
        first = false;
    }
    str << "}";
    return str.str();
}

namespace
{
const boolean_object false_obj {/*val=*/false};
const boolean_object true_obj {/*val=*/true};
const null_object null_obj;
}  // namespace

auto native_true() -> const object*
{
    return &true_obj;
}

auto native_false() -> const object*
{
    return &false_obj;
}

auto native_null() -> const object*
{
    return &null_obj;
}
