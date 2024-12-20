#include <algorithm>
#include <cmath>
#include <cstdint>
#include <ios>
#include <iterator>
#include <ostream>
#include <sstream>
#include <string>
#include <variant>

#include "object.hpp"

#include <ast/builtin_function_expression.hpp>
#include <ast/callable_expression.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <overloaded.hpp>

#include "util.hpp"

namespace
{
const boolean_object false_obj {/*val=*/false};
const boolean_object true_obj {/*val=*/true};
const null_object null_obj;

auto invert_bool_object(const object* b) -> const object*
{
    if (b == &false_obj) {
        return &true_obj;
    }
    if (b == &true_obj) {
        return &false_obj;
    }
    return nullptr;
}

auto object_eq(const object& lhs, const object& rhs) -> bool
{
    return (lhs == rhs) == &true_obj;
}

constexpr auto epsilon = 1e-9;

auto are_almost_equal(double a, double b) -> bool
{
    return std::fabs(a - b) < epsilon;
}
}  // namespace

auto native_bool_to_object(bool val) -> const object*
{
    if (val) {
        return &true_obj;
    }
    return &false_obj;
}

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

auto object::operator!=(const object& other) const -> const object*
{
    return invert_bool_object(*this == other);
}

using enum object::object_type;

builtin_object::builtin_object(const builtin_function_expression* bltn)

    : function_object {bltn, nullptr}
    , builtin {bltn}
{
}

auto operator<<(std::ostream& ostrm, object::object_type type) -> std::ostream&
{
    using enum object::object_type;
    switch (type) {
        case integer:
            return ostrm << "integer";
        case decimal:
            return ostrm << "decimal";
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

auto string_object::operator==(const object& other) const -> const object*
{
    return native_bool_to_object(other.is(type()) && other.as<string_object>()->value == value);
}

auto string_object::operator<(const object& other) const -> const object*
{
    if (other.is(type())) {
        return native_bool_to_object(value < other.as<string_object>()->value);
    }
    return nullptr;
}

auto string_object::operator>(const object& other) const -> const object*
{
    if (other.is(type())) {
        return native_bool_to_object(value > other.as<string_object>()->value);
    }
    return nullptr;
}

auto string_object::operator+(const object& other) const -> const object*
{
    if (other.is(string)) {
        return make<string_object>(value + other.as<string_object>()->value);
    }
    return nullptr;
}

auto string_object::operator*(const object& other) const -> const object*
{
    if (other.is(integer)) {
        return evaluate_sequence_mul(this, &other);
    }
    return nullptr;
}

auto boolean_object::hash_key() const -> hash_key_type
{
    return value;
}

auto boolean_object::operator==(const object& other) const -> const object*
{
    return native_bool_to_object(other.is(type()) && other.as<boolean_object>()->value == value);
}

auto boolean_object::operator<(const object& other) const -> const object*
{
    if (other.is(boolean)) {
        return native_bool_to_object(static_cast<int>(value) < static_cast<int>(other.as<boolean_object>()->value));
    }
    return nullptr;
}

auto boolean_object::operator>(const object& other) const -> const object*
{
    if (other.is(boolean)) {
        return native_bool_to_object(static_cast<int>(value) > static_cast<int>(other.as<boolean_object>()->value));
    }
    return nullptr;
}

auto integer_object::hash_key() const -> hash_key_type
{
    return value;
}

auto integer_object::operator==(const object& other) const -> const object*
{
    if (other.is(type())) {
        return native_bool_to_object(other.as<integer_object>()->value == value);
    }
    return nullptr;
}

auto integer_object::operator+(const object& other) const -> const object*
{
    if (other.is(integer)) {
        return make<integer_object>(value + other.as<integer_object>()->value);
    }
    if (other.is(decimal)) {
        return make<decimal_object>(static_cast<decimal_object::value_type>(value) + other.as<decimal_object>()->value);
    }
    return nullptr;
}

auto integer_object::operator-(const object& other) const -> const object*
{
    if (other.is(integer)) {
        return make<integer_object>(value - other.as<integer_object>()->value);
    }
    if (other.is(decimal)) {
        return make<decimal_object>(static_cast<decimal_object::value_type>(value) - other.as<decimal_object>()->value);
    }
    return nullptr;
}

auto integer_object::operator*(const object& other) const -> const object*
{
    if (other.is(integer)) {
        return make<integer_object>(value * other.as<integer_object>()->value);
    }
    if (other.is(decimal)) {
        return make<decimal_object>(static_cast<decimal_object::value_type>(value) * other.as<decimal_object>()->value);
    }
    if (other.is(array) || other.is(string)) {
        return evaluate_sequence_mul(this, &other);
    }
    return nullptr;
}

auto integer_object::operator/(const object& other) const -> const object*
{
    if (other.is(integer)) {
        const auto other_value = other.as<integer_object>()->value;
        if (other_value == 0) {
            return make_error("division by zero");
        }
        return make<integer_object>(value / other_value);
    }
    if (other.is(decimal)) {
        return make<decimal_object>(static_cast<decimal_object::value_type>(value) / other.as<decimal_object>()->value);
    }
    return nullptr;
}

auto integer_object::operator<(const object& other) const -> const object*
{
    if (other.is(integer)) {
        return native_bool_to_object(value < other.as<integer_object>()->value);
    }
    if (other.is(decimal)) {
        return native_bool_to_object(static_cast<decimal_object::value_type>(value)
                                     < other.as<decimal_object>()->value);
    }
    return nullptr;
}

auto integer_object::operator>(const object& other) const -> const object*
{
    if (other.is(integer)) {
        return native_bool_to_object(value > other.as<integer_object>()->value);
    }
    if (other.is(decimal)) {
        return native_bool_to_object(static_cast<decimal_object::value_type>(value)
                                     > other.as<decimal_object>()->value);
    }
    return nullptr;
}

auto decimal_object::operator==(const object& other) const -> const object*
{
    if (other.is(type())) {
        return native_bool_to_object(are_almost_equal(value, other.as<decimal_object>()->value));
    }
    return nullptr;
}

auto decimal_object::operator+(const object& other) const -> const object*
{
    if (other.is(decimal)) {
        return make<decimal_object>(value + other.as<decimal_object>()->value);
    }
    if (other.is(integer)) {
        return make<decimal_object>(value + static_cast<decimal_object::value_type>(other.as<integer_object>()->value));
    }
    return nullptr;
}

auto decimal_object::operator-(const object& other) const -> const object*
{
    if (other.is(decimal)) {
        return make<decimal_object>(value - other.as<decimal_object>()->value);
    }
    if (other.is(integer)) {
        return make<decimal_object>(value - static_cast<decimal_object::value_type>(other.as<integer_object>()->value));
    }
    return nullptr;
}

auto decimal_object::operator*(const object& other) const -> const object*
{
    if (other.is(decimal)) {
        return make<decimal_object>(value * other.as<decimal_object>()->value);
    }
    if (other.is(integer)) {
        return make<decimal_object>(value * static_cast<decimal_object::value_type>(other.as<integer_object>()->value));
    }
    return nullptr;
}

auto decimal_object::operator/(const object& other) const -> const object*
{
    if (other.is(decimal)) {
        return make<decimal_object>(value / other.as<decimal_object>()->value);
    }
    if (other.is(integer)) {
        return make<decimal_object>(value / static_cast<decimal_object::value_type>(other.as<integer_object>()->value));
    }
    return nullptr;
}

auto decimal_object::operator<(const object& other) const -> const object*
{
    if (other.is(decimal)) {
        return native_bool_to_object(value < other.as<decimal_object>()->value);
    }
    if (other.is(integer)) {
        return native_bool_to_object(value
                                     < static_cast<decimal_object::value_type>(other.as<integer_object>()->value));
    }
    return nullptr;
}

auto decimal_object::operator>(const object& other) const -> const object*
{
    if (other.is(decimal)) {
        return native_bool_to_object(value > other.as<decimal_object>()->value);
    }
    if (other.is(integer)) {
        return native_bool_to_object(value
                                     > static_cast<decimal_object::value_type>(other.as<integer_object>()->value));
    }
    return nullptr;
}

auto builtin_object::inspect() const -> std::string
{
    return fmt::format("builtin {}({}){{...}}", builtin->name, fmt::join(builtin->parameters, ", "));
}

auto function_object::inspect() const -> std::string
{
    return callable->string();
}

auto error_object::operator==(const object& other) const -> const object*
{
    if (other.is(type())) {
        return native_bool_to_object(message == other.as<error_object>()->message);
    }
    return nullptr;
}

auto array_object::inspect() const -> std::string
{
    std::ostringstream strm;
    strm << "[";
    for (bool first = true; const auto* const element : value) {
        if (!first) {
            strm << ", ";
        }
        strm << fmt::format("{}", element->inspect());
        first = false;
    }
    strm << "]";
    return strm.str();
}

auto array_object::operator==(const object& other) const -> const object*
{
    if (other.is(type())) {
        const auto& other_value = other.as<array_object>()->value;
        if (other_value.size() != value.size()) {
            return native_false();
        }
        const auto eq = std::equal(value.cbegin(),
                                   value.cend(),
                                   other_value.cbegin(),
                                   other_value.cend(),
                                   [](const object* a, const object* b) { return object_eq(*a, *b); });
        return native_bool_to_object(eq);
    }
    return nullptr;
}

auto array_object::operator*(const object& other) const -> const object*
{
    if (other.is(integer)) {
        return evaluate_sequence_mul(this, &other);
    }
    return nullptr;
}

auto array_object::operator+(const object& other) const -> const object*
{
    if (other.is(array)) {
        value_type concat = value;
        const auto& other_value = other.as<array_object>()->value;
        std::copy(other_value.cbegin(), other_value.cend(), std::back_inserter(concat));
        return make<array_object>(std::move(concat));
    }
    return nullptr;
}

auto operator<<(std::ostream& strm, const hashable_object::hash_key_type& t) -> std::ostream&
{
    std::visit(
        overloaded {
            [&](int64_t val) { strm << val; },
            [&](const std::string& val) { strm << '"' << val << '"'; },
            [&](bool val) { strm << std::boolalpha << val; },
        },
        t);
    return strm;
}

auto hash_object::inspect() const -> std::string
{
    std::ostringstream strm;
    strm << "{";
    for (bool first = true; const auto& [key, value] : value) {
        if (!first) {
            strm << ", ";
        }
        strm << key;
        strm << ": " << value->inspect();
        first = false;
    }
    strm << "}";
    return strm.str();
}

auto hash_object::operator==(const object& other) const -> const object*
{
    if (other.is(type())) {
        const auto& other_value = other.as<hash_object>()->value;
        if (other_value.size() != value.size()) {
            return native_false();
        }
        const auto eq = std::all_of(value.cbegin(),
                                    value.cend(),
                                    [&other_value](const auto& pair)
                                    {
                                        const auto& [key, value] = pair;
                                        auto it = other_value.find(key);
                                        return it != other_value.cend() && object_eq(*(it->second), *value);
                                    });
        return native_bool_to_object(eq);
    }
    return nullptr;
}

auto null_object::operator==(const object& other) const -> const object*
{
    if (other.is(type())) {
        return native_true();
    }
    return nullptr;
}
