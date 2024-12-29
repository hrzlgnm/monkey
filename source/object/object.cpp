#include <algorithm>
#include <cmath>
#include <cstdint>
#include <ios>
#include <iterator>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>
#include <variant>

#include "object.hpp"

#include <ast/util.hpp>
#include <builtin/builtin.hpp>
#include <code/code.hpp>
#include <doctest/doctest.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <gc.hpp>
#include <overloaded.hpp>

using enum object::object_type;

namespace
{

auto invert_boolean_object(const object* b) -> const object*
{
    if (b == fals()) {
        return tru();
    }
    if (b == tru()) {
        return fals();
    }
    return nullptr;
}

auto object_eq(const object& lhs, const object& rhs) -> bool
{
    return (lhs == rhs) == tru();
}

constexpr auto epsilon = 1e-9;

auto are_almost_equal(double a, double b) -> bool
{
    return std::fabs(a - b) < epsilon;
}

template<typename T>
auto eq_helper(const T* t, const object& other) -> const object*
{
    if constexpr (std::is_same_v<T, decimal_object>) {
        return native_bool_to_object(other.is(t->type()) && are_almost_equal(t->value, other.val<decimal_object>()));
    }
    return native_bool_to_object(other.is(t->type()) && t->value == other.as<T>()->value);
}

template<typename T>
auto value_eq_helper(const T& lhs, const T& rhs) -> const object*
{
    if constexpr (std::is_same_v<T, double>) {
        return native_bool_to_object(are_almost_equal(lhs, rhs));
    }
    return native_bool_to_object(lhs == rhs);
}

template<typename T>
auto value_gt_helper(const T& lhs, const T& rhs) -> const object*
{
    return native_bool_to_object(lhs > rhs);
}

template<typename T>
auto value_ge_helper(const T& lhs, const T& rhs) -> const object*
{
    return native_bool_to_object(lhs >= rhs);
}

template<typename T>
auto gt_helper(const T* t, const object& other) -> const object*
{
    if (other.is(t->type())) {
        return native_bool_to_object(t->value > other.as<T>()->value);
    }
    return nullptr;
}

template<typename T>
auto ge_helper(const T* t, const object& other) -> const object*
{
    if (other.is(t->type())) {
        return native_bool_to_object(t->value >= other.as<T>()->value);
    }
    return nullptr;
}

template<typename T>
auto multiply_sequence_helper(const T* source, integer_object::value_type count) -> const object*
{
    typename T::value_type target;
    for (integer_object::value_type i = 0; i < count; i++) {
        std::copy(source->value.cbegin(), source->value.cend(), std::back_inserter(target));
    }
    return make<T>(std::move(target));
}

auto math_mod(integer_object::value_type lhs, integer_object::value_type rhs) -> integer_object::value_type
{
    return ((lhs % rhs) + rhs) % rhs;
}

auto math_mod(decimal_object::value_type lhs, decimal_object::value_type rhs) -> decimal_object::value_type
{
    return std::fmod(std::fmod(lhs, rhs) + rhs, rhs);
}
}  // namespace

auto native_bool_to_object(bool val) -> const object*
{
    if (val) {
        return tru();
    }
    return fals();
}

auto tru() -> const object*
{
    static const boolean_object true_obj {/*val=*/true};
    return &true_obj;
}

auto fals() -> const object*
{
    static const boolean_object false_obj {/*val=*/false};
    return &false_obj;
}

auto null() -> const object*
{
    static const struct null_object null_obj;
    return &null_obj;
}

auto brake() -> const object*
{
    static const struct break_object break_obj;
    return &break_obj;
}

auto cont() -> const object*
{
    static const struct continue_object continue_obj;
    return &continue_obj;
}

auto object::operator!=(const object& other) const -> const object*
{
    return invert_boolean_object(*this == other);
}

auto object::operator&&(const object& other) const -> const object*
{
    return native_bool_to_object(is_truthy() && other.is_truthy());
}

auto object::operator||(const object& other) const -> const object*
{
    return native_bool_to_object(is_truthy() || other.is_truthy());
}

builtin_object::builtin_object(const struct builtin* bltn)

    : builtin {bltn}
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
        case return_value:
            return ostrm << "return_value";
    }
    return ostrm << "unknown " << std::hex << static_cast<int>(type);
}

auto string_object::hash_key() const -> key_type
{
    return value;
}

auto string_object::operator==(const object& other) const -> const object*
{
    return eq_helper(this, other);
}

auto string_object::operator>(const object& other) const -> const object*
{
    return gt_helper(this, other);
}

auto string_object::operator>=(const object& other) const -> const object*
{
    return ge_helper(this, other);
}

auto string_object::operator+(const object& other) const -> const object*
{
    if (other.is(string)) {
        return make<string_object>(value + other.val<string_object>());
    }
    return nullptr;
}

auto string_object::operator*(const object& other) const -> const object*
{
    if (other.is(integer)) {
        return multiply_sequence_helper(this, other.val<integer_object>());
    }
    return nullptr;
}

auto boolean_object::hash_key() const -> key_type
{
    return value;
}

auto boolean_object::operator==(const object& other) const -> const object*
{
    if (other.is(decimal)) {
        return value_eq_helper(value_to<decimal_object>(), other.val<decimal_object>());
    }
    if (other.is(integer)) {
        return value_eq_helper(value_to<integer_object>(), other.val<integer_object>());
    }
    return eq_helper(this, other);
}

auto boolean_object::operator>(const object& other) const -> const object*
{
    if (other.is(integer)) {
        return value_gt_helper(value_to<integer_object>(), other.val<integer_object>());
    }
    if (other.is(decimal)) {
        return value_gt_helper(value_to<decimal_object>(), other.val<decimal_object>());
    }
    return gt_helper(this, other);
}

auto boolean_object::operator>=(const object& other) const -> const object*
{
    if (other.is(integer)) {
        return value_ge_helper(value_to<integer_object>(), other.val<integer_object>());
    }
    if (other.is(decimal)) {
        return value_ge_helper(value_to<decimal_object>(), other.val<decimal_object>());
    }
    return ge_helper(this, other);
}

auto boolean_object::operator+(const object& other) const -> const object*
{
    if (other.is(integer)) {
        return other + *this;
    }
    if (other.is(decimal)) {
        return other + *this;
    }
    if (other.is(boolean)) {
        return make<integer_object>(value_to<integer_object>()
                                    + other.as<boolean_object>()->value_to<integer_object>());
    }
    return nullptr;
}

auto boolean_object::operator-(const object& other) const -> const object*
{
    if (other.is(integer)) {
        return make<integer_object>(value_to<integer_object>() - other.val<integer_object>());
    }
    if (other.is(decimal)) {
        return make<decimal_object>(value_to<decimal_object>() - other.val<decimal_object>());
    }
    if (other.is(boolean)) {
        return make<integer_object>(value_to<integer_object>()
                                    - other.as<boolean_object>()->value_to<integer_object>());
    }
    return nullptr;
}

auto boolean_object::operator*(const object& other) const -> const object*
{
    if (other.is(integer)) {
        return other * *this;
    }
    if (other.is(decimal)) {
        return other * *this;
    }
    if (other.is(boolean)) {
        return make<integer_object>(value_to<integer_object>()
                                    - other.as<boolean_object>()->value_to<integer_object>());
    }
    return nullptr;
}

auto boolean_object::operator/(const object& other) const -> const object*
{
    if (other.is(integer)) {
        const auto other_value = other.val<integer_object>();
        if (other_value == 0) {
            return make_error("division by zero");
        }
        return make<decimal_object>(value_to<decimal_object>()
                                    / other.as<integer_object>()->value_to<decimal_object>());
    }
    if (other.is(boolean)) {
        const auto other_value = other.as<boolean_object>()->value_to<integer_object>();
        if (other_value == 0) {
            return make_error("division by zero");
        }
        return make<decimal_object>(value_to<decimal_object>()
                                    / other.as<boolean_object>()->value_to<decimal_object>());
    }
    if (other.is(decimal)) {
        return make<decimal_object>(value_to<decimal_object>() / other.val<decimal_object>());
    }
    return nullptr;
}

auto boolean_object::operator%(const object& other) const -> const object*
{
    if (other.is(integer)) {
        const auto other_value = other.val<integer_object>();
        if (other_value == 0) {
            return make_error("division by zero");
        }
        return make<integer_object>(math_mod(value_to<integer_object>(), other_value));
    }
    if (other.is(boolean)) {
        const auto other_value = other.as<boolean_object>()->value_to<integer_object>();
        if (other_value == 0) {
            return make_error("division by zero");
        }
        return make<integer_object>(math_mod(value_to<integer_object>(), other_value));
    }
    if (other.is(decimal)) {
        return make<decimal_object>(math_mod(value_to<decimal_object>(), other.val<decimal_object>()));
    }
    return nullptr;
}

auto boolean_object::operator&(const object& other) const -> const object*
{
    if (other.is(boolean)) {
        return make<boolean_object>(value & other.val<boolean_object>());
    }
    if (other.is(integer)) {
        return other & *this;
    }
    return nullptr;
}

auto boolean_object::operator|(const object& other) const -> const object*
{
    if (other.is(boolean)) {
        return make<boolean_object>(value | other.val<boolean_object>());
    }
    if (other.is(integer)) {
        return other | *this;
    }
    return nullptr;
}

auto boolean_object::operator^(const object& other) const -> const object*
{
    if (other.is(boolean)) {
        return make<boolean_object>(value ^ other.val<boolean_object>());
    }
    if (other.is(integer)) {
        return other ^ *this;
    }
    return nullptr;
}

auto boolean_object::operator<<(const object& other) const -> const object*
{
    if (other.is(boolean)) {
        return make<integer_object>(value_to<integer_object>() << other.val<boolean_object>());
    }
    if (other.is(integer)) {
        return make<integer_object>(value_to<integer_object>() << other.val<integer_object>());
    }
    return nullptr;
}

auto boolean_object::operator>>(const object& other) const -> const object*
{
    if (other.is(boolean)) {
        return make<integer_object>(value_to<integer_object>() >> other.val<boolean_object>());
    }
    if (other.is(integer)) {
        return make<integer_object>(value_to<integer_object>() >> other.val<integer_object>());
    }
    return nullptr;
}

auto integer_object::hash_key() const -> key_type
{
    return value;
}

auto integer_object::operator==(const object& other) const -> const object*
{
    if (other.is(decimal)) {
        return value_eq_helper(value_to<decimal_object>(), other.val<decimal_object>());
    }
    if (other.is(boolean)) {
        return value_eq_helper(value, other.as<boolean_object>()->value_to<integer_object>());
    }
    return eq_helper(this, other);
}

auto integer_object::operator>(const object& other) const -> const object*
{
    if (other.is(boolean)) {
        return value_gt_helper(value_to<integer_object>(), other.as<boolean_object>()->value_to<integer_object>());
    }
    if (other.is(decimal)) {
        return value_gt_helper(value_to<decimal_object>(), other.val<decimal_object>());
    }
    return gt_helper(this, other);
}

auto integer_object::operator>=(const object& other) const -> const object*
{
    if (other.is(boolean)) {
        return value_ge_helper(value_to<integer_object>(), other.as<boolean_object>()->value_to<integer_object>());
    }
    if (other.is(decimal)) {
        return value_ge_helper(value_to<decimal_object>(), other.val<decimal_object>());
    }
    return ge_helper(this, other);
}

auto integer_object::operator+(const object& other) const -> const object*
{
    if (other.is(integer)) {
        return make<integer_object>(value + other.val<integer_object>());
    }
    if (other.is(boolean)) {
        return make<integer_object>(value + other.as<boolean_object>()->value_to<integer_object>());
    }
    if (other.is(decimal)) {
        return make<decimal_object>(other.val<decimal_object>() + value_to<decimal_object>());
    }
    return nullptr;
}

auto integer_object::operator-(const object& other) const -> const object*
{
    if (other.is(integer)) {
        return make<integer_object>(value - other.val<integer_object>());
    }
    if (other.is(boolean)) {
        return make<integer_object>(value - other.as<boolean_object>()->value_to<integer_object>());
    }
    if (other.is(decimal)) {
        return make<decimal_object>(value_to<decimal_object>() - other.val<decimal_object>());
    }
    return nullptr;
}

auto integer_object::operator*(const object& other) const -> const object*
{
    if (other.is(integer)) {
        return make<integer_object>(value * other.val<integer_object>());
    }
    if (other.is(boolean)) {
        return make<integer_object>(value * other.as<boolean_object>()->value_to<integer_object>());
    }
    if (other.is(decimal)) {
        return make<decimal_object>(value_to<decimal_object>() * other.val<decimal_object>());
    }
    if (other.is(array)) {
        return multiply_sequence_helper(other.as<array_object>(), value);
    }
    if (other.is(string)) {
        return multiply_sequence_helper(other.as<string_object>(), value);
    }

    return nullptr;
}

auto integer_object::operator/(const object& other) const -> const object*
{
    if (other.is(integer)) {
        const auto other_value = other.val<integer_object>();
        if (other_value == 0) {
            return make_error("division by zero");
        }
        return make<decimal_object>(value_to<decimal_object>()
                                    / other.as<integer_object>()->value_to<decimal_object>());
    }
    if (other.is(boolean)) {
        const auto other_value = other.as<boolean_object>()->value_to<integer_object>();
        if (other_value == 0) {
            return make_error("division by zero");
        }
        return make<decimal_object>(value_to<decimal_object>()
                                    / other.as<boolean_object>()->value_to<decimal_object>());
    }
    if (other.is(decimal)) {
        return make<decimal_object>(value_to<decimal_object>() / other.val<decimal_object>());
    }
    return nullptr;
}

auto integer_object::operator%(const object& other) const -> const object*
{
    if (other.is(integer)) {
        const auto other_value = other.val<integer_object>();
        if (other_value == 0) {
            return make_error("division by zero");
        }
        return make<integer_object>(math_mod(value, other_value));
    }
    if (other.is(boolean)) {
        const auto other_value = other.as<boolean_object>()->value_to<integer_object>();
        if (other_value == 0) {
            return make_error("division by zero");
        }
        return make<integer_object>(math_mod(value, other_value));
    }
    if (other.is(decimal)) {
        return make<decimal_object>(math_mod(value_to<decimal_object>(), other.val<decimal_object>()));
    }
    return nullptr;
}

auto integer_object::operator|(const object& other) const -> const object*
{
    if (other.is(integer)) {
        return make<integer_object>(value | other.val<integer_object>());
    }
    if (other.is(boolean)) {
        const auto other_value = other.as<boolean_object>()->value_to<integer_object>();
        return make<integer_object>(value | other_value);
    }
    return nullptr;
}

auto integer_object::operator^(const object& other) const -> const object*
{
    if (other.is(integer)) {
        return make<integer_object>(value ^ other.val<integer_object>());
    }
    if (other.is(boolean)) {
        const auto other_value = other.as<boolean_object>()->value_to<integer_object>();
        return make<integer_object>(value ^ other_value);
    }
    return nullptr;
}

auto integer_object::operator<<(const object& other) const -> const object*
{
    if (other.is(integer)) {
        return make<integer_object>(value << other.val<integer_object>());
    }
    if (other.is(boolean)) {
        const auto other_value = other.as<boolean_object>()->value_to<integer_object>();
        return make<integer_object>(value << other_value);
    }
    return nullptr;
}

auto integer_object::operator>>(const object& other) const -> const object*
{
    if (other.is(integer)) {
        return make<integer_object>(value >> other.val<integer_object>());
    }
    if (other.is(boolean)) {
        const auto other_value = other.as<boolean_object>()->value_to<integer_object>();
        return make<integer_object>(value >> other_value);
    }
    return nullptr;
}

auto integer_object::operator&(const object& other) const -> const object*
{
    if (other.is(integer)) {
        return make<integer_object>(value & other.val<integer_object>());
    }
    if (other.is(boolean)) {
        const auto other_value = other.as<boolean_object>()->value_to<integer_object>();
        return make<integer_object>(value & other_value);
    }
    return nullptr;
}

auto decimal_object::operator==(const object& other) const -> const object*
{
    if (other.is(boolean)) {
        return value_eq_helper(value, other.as<boolean_object>()->value_to<decimal_object>());
    }
    if (other.is(integer)) {
        return value_eq_helper(value, other.as<integer_object>()->value_to<decimal_object>());
    }
    return eq_helper(this, other);
}

auto decimal_object::operator+(const object& other) const -> const object*
{
    if (other.is(boolean)) {
        return make<decimal_object>(value + other.as<boolean_object>()->value_to<decimal_object>());
    }
    if (other.is(integer)) {
        return make<decimal_object>(value + other.as<integer_object>()->value_to<decimal_object>());
    }
    if (other.is(decimal)) {
        return make<decimal_object>(value + other.val<decimal_object>());
    }
    return nullptr;
}

auto decimal_object::operator-(const object& other) const -> const object*
{
    if (other.is(boolean)) {
        return make<decimal_object>(value - other.as<boolean_object>()->value_to<decimal_object>());
    }
    if (other.is(integer)) {
        return make<decimal_object>(value - other.as<integer_object>()->value_to<decimal_object>());
    }
    if (other.is(decimal)) {
        return make<decimal_object>(value - other.val<decimal_object>());
    }
    return nullptr;
}

auto decimal_object::operator*(const object& other) const -> const object*
{
    if (other.is(boolean)) {
        return make<decimal_object>(value * other.as<boolean_object>()->value_to<decimal_object>());
    }
    if (other.is(integer)) {
        return make<decimal_object>(value * other.as<integer_object>()->value_to<decimal_object>());
    }
    if (other.is(decimal)) {
        return make<decimal_object>(value * other.val<decimal_object>());
    }
    return nullptr;
}

auto decimal_object::operator/(const object& other) const -> const object*
{
    if (other.is(boolean)) {
        return make<decimal_object>(value / other.as<boolean_object>()->value_to<decimal_object>());
    }
    if (other.is(integer)) {
        return make<decimal_object>(value / other.as<integer_object>()->value_to<decimal_object>());
    }
    if (other.is(decimal)) {
        return make<decimal_object>(value / other.val<decimal_object>());
    }
    return nullptr;
}

auto decimal_object::operator%(const object& other) const -> const object*
{
    if (other.is(boolean)) {
        return make<decimal_object>(math_mod(value, other.as<boolean_object>()->value_to<decimal_object>()));
    }
    if (other.is(integer)) {
        return make<decimal_object>(math_mod(value, other.as<integer_object>()->value_to<decimal_object>()));
    }
    if (other.is(decimal)) {
        return make<decimal_object>(math_mod(value, other.val<decimal_object>()));
    }
    return nullptr;
}

auto decimal_object::operator>(const object& other) const -> const object*
{
    if (other.is(boolean)) {
        return value_gt_helper(value, other.as<boolean_object>()->value_to<decimal_object>());
    }
    if (other.is(integer)) {
        return value_gt_helper(value, other.as<integer_object>()->value_to<decimal_object>());
    }
    return gt_helper(this, other);
}

auto decimal_object::operator>=(const object& other) const -> const object*
{
    if (other.is(boolean)) {
        return value_ge_helper(value, other.as<boolean_object>()->value_to<decimal_object>());
    }
    if (other.is(integer)) {
        return value_ge_helper(value, other.as<integer_object>()->value_to<decimal_object>());
    }
    return ge_helper(this, other);
}

auto object_floor_div(const object* lhs, const object* rhs) -> const object*
{
    const auto* div = (*lhs / *rhs);
    if (div->is(decimal)) {
        return make<decimal_object>(std::floor(div->val<decimal_object>()));
    }
    return div;
}

auto builtin_object::inspect() const -> std::string
{
    return fmt::format("builtin {}({}){{...}}", builtin->name, fmt::join(builtin->parameters, ", "));
}

auto return_value_object::inspect() const -> std::string
{
    return fmt::format("{}", return_value->inspect());
}

auto function_object::inspect() const -> std::string
{
    return fmt::format("fn({}) {{\n{}\n}}", join(parameters, ", "), body->string());
}

auto error_object::operator==(const object& other) const -> const object*
{
    return eq_helper(this, other);
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
            return fals();
        }
        const auto eq = std::equal(value.cbegin(),
                                   value.cend(),
                                   other_value.cbegin(),
                                   other_value.cend(),
                                   [](const object* a, const object* b) { return object_eq(*a, *b); });
        return native_bool_to_object(eq);
    }
    return fals();
}

auto array_object::operator*(const object& other) const -> const object*
{
    if (other.is(integer)) {
        return multiply_sequence_helper(this, other.val<integer_object>());
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

auto operator<<(std::ostream& strm, const hashable::key_type& t) -> std::ostream&
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
            return fals();
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
    return object::operator==(other);
}

auto hash_object::operator+(const object& other) const -> const object*
{
    if (other.is(hash)) {
        value_type concat = value;
        const auto& other_value = other.as<hash_object>()->value;
        for (const auto& pair : other_value) {
            concat.insert_or_assign(pair.first, pair.second);
        }
        return make<hash_object>(std::move(concat));
    }
    return nullptr;
}

auto null_object::operator==(const object& other) const -> const object*
{
    return native_bool_to_object(other.is(type()));
}

auto compiled_function_object::inspect() const -> std::string
{
    return "{<code...>}";
}

[[nodiscard]] auto closure_object::as_mutable() const -> closure_object*
{
    // NOLINTBEGIN(cppcoreguidelines-pro-type-const-cast)
    return const_cast<closure_object*>(this);
    // NOLINTEND(cppcoreguidelines-pro-type-const-cast)
}

auto closure_object::inspect() const -> std::string
{
    return fmt::format("closure[{}]", static_cast<const void*>(fn));
}

namespace
{
// NOLINTBEGIN(*)

auto check_op_defined(const object* res, std::string_view op, const object& lhs, const object& rhs) -> bool
{
    if (res == nullptr) {
        INFO("operator ", lhs.type(), " ", op, " ", rhs.type(), " is not defined ");
        CHECK(res != nullptr);
        return false;
    }
    return true;
}

auto check_eq(const object& lhs, const object& rhs) -> void
{
    const auto* res = lhs == rhs;
    if (!check_op_defined(res, "==", lhs, rhs)) {
        return;
    }
    INFO("expected ",
         lhs.inspect(),
         " != ",
         rhs.inspect(),
         " to be: ",
         tru()->inspect(),
         " with type: ",
         tru()->type(),
         " got: ",
         res->inspect(),
         " with type: ",
         res->type(),
         " instead");
    CHECK((*res == *tru()) == tru());
}

auto check_ne(const object& lhs, const object& rhs) -> void
{
    const auto* res = lhs != rhs;
    if (!check_op_defined(res, "!=", lhs, rhs)) {
        return;
    }
    INFO("expected ",
         lhs.inspect(),
         " != ",
         rhs.inspect(),
         " to be: ",
         tru()->inspect(),
         " with type: ",
         tru()->type(),
         " got: ",
         res->inspect(),
         " with type: ",
         res->type(),
         " instead");
    CHECK((*res == *tru()) == tru());
}

auto check_add(const object& lhs, const object& rhs, const object& expected) -> void
{
    const auto* res = lhs + rhs;
    if (!check_op_defined(res, "+", lhs, rhs)) {
        return;
    }
    INFO("expected ",
         lhs.inspect(),
         " + ",
         rhs.inspect(),
         " to become: ",
         expected.inspect(),
         " with type: ",
         expected.type(),
         " got: ",
         res->inspect(),
         " with type: ",
         res->type());
    CHECK((*res == expected) == tru());
}

auto check_sub(const object& lhs, const object& rhs, const object& expected) -> void
{
    const auto* res = lhs - rhs;
    if (!check_op_defined(res, "-", lhs, rhs)) {
        return;
    }
    INFO("expected ",
         lhs.inspect(),
         " - ",
         rhs.inspect(),
         " to become: ",
         expected.inspect(),
         " with type: ",
         expected.type(),
         " got: ",
         res->inspect(),
         " with type: ",
         res->type());
    CHECK((*res == expected) == tru());
}

auto check_mul(const object& lhs, const object& rhs, const object& expected) -> void
{
    const auto* res = lhs * rhs;
    if (!check_op_defined(res, "*", lhs, rhs)) {
        return;
    }
    INFO("expected ",
         lhs.inspect(),
         " * ",
         rhs.inspect(),
         " to become: ",
         expected.inspect(),
         " with type: ",
         expected.type(),
         " got: ",
         res->inspect(),
         " with type: ",
         res->type());
    CHECK((*res == expected) == tru());
}

auto check_nan(const object* expected) -> void
{
    REQUIRE(expected != nullptr);
    REQUIRE(expected->is(decimal));
    CHECK(std::isnan(expected->val<decimal_object>()));
}

auto check_inf(const object* expected) -> void
{
    REQUIRE(expected != nullptr);
    REQUIRE(expected->is(decimal));
    CHECK(std::isinf(expected->val<decimal_object>()));
}

auto check_div(const object& lhs, const object& rhs, const object& expected) -> void
{
    const auto* res = lhs / rhs;
    if (!check_op_defined(res, "/", lhs, rhs)) {
        return;
    }
    INFO("expected ",
         lhs.inspect(),
         " / ",
         rhs.inspect(),
         " to become: ",
         expected.inspect(),
         " with type: ",
         expected.type(),
         " got: ",
         res->inspect(),
         " with type: ",
         res->type());
    CHECK((*res == expected) == tru());
}

auto check_floor_div(const object& lhs, const object& rhs, const object& expected) -> void
{
    const auto* res = object_floor_div(&lhs, &rhs);
    if (!check_op_defined(res, "//", lhs, rhs)) {
        return;
    }
    INFO("expected ",
         lhs.inspect(),
         " // ",
         rhs.inspect(),
         " to become: ",
         expected.inspect(),
         " with type: ",
         expected.type(),
         " got: ",
         res->inspect(),
         " with type: ",
         res->type());
    CHECK((*res == expected) == tru());
}

auto check_mod(const object& lhs, const object& rhs, const object& expected) -> void
{
    const auto* res = lhs % rhs;
    if (!check_op_defined(res, "%", lhs, rhs)) {
        return;
    }
    INFO("expected ",
         lhs.inspect(),
         " % ",
         rhs.inspect(),
         " to become: ",
         expected.inspect(),
         " with type: ",
         expected.type(),
         " got: ",
         res->inspect(),
         " with type: ",
         res->type());
    CHECK((*res == expected) == tru());
}

auto check_bit_and(const object& lhs, const object& rhs, const object& expected) -> void
{
    const auto* res = lhs & rhs;
    if (!check_op_defined(res, "&", lhs, rhs)) {
        return;
    }
    INFO("expected ",
         lhs.inspect(),
         " & ",
         rhs.inspect(),
         " to become: ",
         expected.inspect(),
         " with type: ",
         expected.type(),
         " got: ",
         res->inspect(),
         " with type: ",
         res->type());
    CHECK((*res == expected) == tru());
}

auto check_bit_or(const object& lhs, const object& rhs, const object& expected) -> void
{
    const auto* res = lhs | rhs;
    if (!check_op_defined(res, "|", lhs, rhs)) {
        return;
    }
    INFO("expected ",
         lhs.inspect(),
         " | ",
         rhs.inspect(),
         " to become: ",
         expected.inspect(),
         " with type: ",
         expected.type(),
         " got: ",
         res->inspect(),
         " with type: ",
         res->type());
    CHECK((*res == expected) == tru());
}

auto check_bit_xor(const object& lhs, const object& rhs, const object& expected) -> void
{
    const auto* res = lhs ^ rhs;
    if (!check_op_defined(res, "^", lhs, rhs)) {
        return;
    }
    INFO("expected ",
         lhs.inspect(),
         " ^ ",
         rhs.inspect(),
         " to become: ",
         expected.inspect(),
         " with type: ",
         expected.type(),
         " got: ",
         res->inspect(),
         " with type: ",
         res->type());
    CHECK((*res == expected) == tru());
}

auto check_bit_shl(const object& lhs, const object& rhs, const object& expected) -> void
{
    const auto* res = lhs << rhs;
    if (!check_op_defined(res, "<<", lhs, rhs)) {
        return;
    }
    INFO("expected ",
         lhs.inspect(),
         " << ",
         rhs.inspect(),
         " to become: ",
         expected.inspect(),
         " with type: ",
         expected.type(),
         " got: ",
         res->inspect(),
         " with type: ",
         res->type());
    CHECK((*res == expected) == tru());
}

auto check_bit_shr(const object& lhs, const object& rhs, const object& expected) -> void
{
    const auto* res = lhs >> rhs;
    if (!check_op_defined(res, ">", lhs, rhs)) {
        return;
    }
    INFO("expected ",
         lhs.inspect(),
         " >> ",
         rhs.inspect(),
         " to become: ",
         expected.inspect(),
         " with type: ",
         expected.type(),
         " got: ",
         res->inspect(),
         " with type: ",
         res->type());
    CHECK((*res == expected) == tru());
}

TEST_SUITE("object tests")
{
    const auto i1 {123};
    const auto i2 {124};
    const auto d1 {12.3};
    const auto d2 {123.1};
    const auto d3 {123};
    const integer_object int_obj {i1};
    const decimal_object dec_obj {d1};
    const string_object str_obj {"str"};
    const boolean_object true_obj {/*val=*/true};
    const boolean_object false_obj {/*val=*/false};
    const null_object null_obj;
    const error_object err_obj {"error"};
    const integer_object int2_obj {i2};
    const array_object array_obj {{&int_obj, &int2_obj}};
    const hash_object hash_obj {{{1, &str_obj}, {2, &true_obj}}};
    const return_value_object ret_obj {&array_obj};
    const function_object function_obj {{}, nullptr, nullptr};
    const builtin_object builtin_obj {builtin::builtins()[0]};
    const compiled_function_object cmpld_obj {{}, 0, 0};
    const closure_object clsr_obj {&cmpld_obj, {}};

    TEST_CASE("is truthy")
    {
        CHECK_FALSE(integer_object {0}.is_truthy());
        CHECK_FALSE(string_object {""}.is_truthy());
        CHECK_FALSE(false_obj.is_truthy());
        CHECK_FALSE(null()->is_truthy());
        CHECK_FALSE(array_object {{}}.is_truthy());
        CHECK_FALSE(hash_object {{}}.is_truthy());
        CHECK_FALSE(decimal_object {0}.is_truthy());
        CHECK(integer_object {1}.is_truthy());
        CHECK(decimal_object {1}.is_truthy());
        CHECK(string_object {"1"}.is_truthy());
        CHECK(true_obj.is_truthy());
        CHECK(array_object {{&int_obj}}.is_truthy());
        CHECK(integer_object {1}.is_truthy());
        CHECK(integer_object {1}.is_truthy());
        CHECK(error_object {{}}.is_truthy());
        CHECK(return_value_object {tru()}.is_truthy());
        CHECK(function_obj.is_truthy());
        CHECK(builtin_obj.is_truthy());
        CHECK(cmpld_obj.is_truthy());
        CHECK(clsr_obj.is_truthy());
    }
    TEST_CASE("type")
    {
        using enum object::object_type;
        CHECK_EQ(int_obj.type(), integer);
        CHECK_EQ(dec_obj.type(), decimal);
        CHECK_EQ(true_obj.type(), boolean);
        CHECK_EQ(str_obj.type(), string);
        CHECK_EQ(err_obj.type(), error);
        CHECK_EQ(array_obj.type(), array);
        CHECK_EQ(hash_obj.type(), hash);
        CHECK_EQ(return_value_object {&int_obj}.type(), return_value);
        CHECK_EQ(function_obj.type(), function);
        CHECK_EQ(cmpld_obj.type(), compiled_function);
        CHECK_EQ(clsr_obj.type(), closure);
        CHECK_EQ(builtin_obj.type(), builtin);
    }
    TEST_CASE("inspect")
    {
        CHECK_EQ(int_obj.inspect(), "123");
        CHECK_EQ(dec_obj.inspect(), "12.3");
        CHECK_EQ(true_obj.inspect(), "true");
        CHECK_EQ(str_obj.inspect(), R"("str")");
        CHECK_EQ(err_obj.inspect(), "ERROR: error");
        CHECK_EQ(brake()->inspect(), "break");
        CHECK_EQ(cont()->inspect(), "continue");
        CHECK_EQ(null()->inspect(), "null");
        CHECK_EQ(array_obj.inspect(), "[123, 124]");
        CHECK_EQ(hash_object {{{1, &str_obj}}}.inspect(), R"({1: "str"})");
        CHECK_EQ(ret_obj.inspect(), R"([123, 124])");
    }

    TEST_CASE("operator ==")
    {
        check_eq(int_obj, integer_object {i1});
        check_eq(dec_obj, decimal_object {d1});
        check_eq(decimal_object {d3}, integer_object {i1});
        check_eq(true_obj, boolean_object {true});
        check_eq(true_obj, integer_object {1});
        check_eq(true_obj, decimal_object {1});
        check_eq(integer_object {1}, true_obj);
        check_eq(decimal_object {1}, true_obj);
        check_eq(false_obj, boolean_object {false});
        check_eq(null_obj, null_object {});
        check_eq(array_obj, array_object {{&int_obj, &int2_obj}});
        check_eq(hash_obj, hash_object {{{2, &true_obj}, {1, &str_obj}}});
    }

    TEST_CASE("operator !=")
    {
        check_ne(int_obj, integer_object {122});
        check_ne(decimal_object {d2}, integer_object {i1});
        check_ne(true_obj, false_obj);
        check_ne(false_obj, null_obj);
        check_ne(null_obj, true_obj);
        check_ne(hash_obj, hash_object {{{2, &str_obj}, {1, &int_obj}}});
        check_ne(array_obj, array_object {{&int2_obj, &int_obj}});
    }

    TEST_CASE("operator >")
    {
        REQUIRE_EQ(int_obj > true_obj, tru());
        REQUIRE_EQ(int_obj > integer_object {122}, tru());
        REQUIRE_EQ(decimal_object {d2} > integer_object {i1}, tru());
        REQUIRE_EQ(decimal_object {d2} > true_obj, tru());
        REQUIRE_EQ(str_obj > string_object {"st"}, tru());
        REQUIRE_EQ(true_obj > decimal_object {0.9}, tru());
        REQUIRE_EQ(true_obj > false_obj, tru());
        REQUIRE_EQ(false_obj > null_obj, nullptr);
    }

    TEST_CASE("operator >=")
    {
        REQUIRE_EQ(int_obj >= true_obj, tru());
        REQUIRE_EQ(true_obj >= true_obj, tru());
        REQUIRE_EQ(int_obj >= integer_object {122}, tru());
        REQUIRE_EQ(int_obj >= int_obj, tru());
        REQUIRE_EQ(decimal_object {d2} >= decimal_object {d2}, tru());
        REQUIRE_EQ(decimal_object {d2} >= true_obj, tru());
        REQUIRE_EQ(str_obj >= string_object {"st"}, tru());
        REQUIRE_EQ(str_obj >= str_obj, tru());
        REQUIRE_EQ(true_obj >= decimal_object {0.9}, tru());
        REQUIRE_EQ(true_obj >= false_obj, tru());
        REQUIRE_EQ(false_obj >= false_obj, tru());
        REQUIRE_EQ(false_obj >= null_obj, nullptr);
        REQUIRE_EQ(false_obj >= str_obj, nullptr);
    }

    TEST_CASE("operator &&")
    {
        REQUIRE_EQ(true_obj && true_obj, tru());
        REQUIRE_EQ(int_obj && true_obj, tru());
        REQUIRE_EQ(decimal_object {d2} && integer_object {i1}, tru());
        REQUIRE_EQ(str_obj && string_object {""}, fals());
        REQUIRE_EQ(true_obj && false_obj, fals());
        REQUIRE_EQ(false_obj && null_obj, fals());
    }

    TEST_CASE("operator ||")
    {
        REQUIRE_EQ(true_obj || true_obj, tru());
        REQUIRE_EQ(int_obj || true_obj, tru());
        REQUIRE_EQ(decimal_object {d2} || integer_object {i1}, tru());
        REQUIRE_EQ(str_obj || string_object {""}, tru());
        REQUIRE_EQ(true_obj || false_obj, tru());
        REQUIRE_EQ(false_obj || null_obj, fals());
    }

    TEST_CASE("operator +")
    {
        check_add(integer_object {1}, integer_object {1}, integer_object {2});
        check_add(decimal_object {1}, integer_object {1}, decimal_object {2});
        check_add(true_obj, integer_object {1}, integer_object {2});
        check_add(true_obj, decimal_object {1}, decimal_object {2});
        check_add(integer_object {1}, true_obj, integer_object {2});
        check_add(decimal_object {1}, true_obj, decimal_object {2});
        check_add(false_obj, decimal_object {1}, decimal_object {1});
        check_add(false_obj, integer_object {1}, integer_object {1});
        check_add(array_obj, array_obj, array_object {{&int_obj, &int2_obj, &int_obj, &int2_obj}});
        check_add(
            hash_obj, hash_object {{{3, &false_obj}}}, hash_object {{{1, &str_obj}, {2, &true_obj}, {3, &false_obj}}});
    }

    TEST_CASE("operator -")
    {
        check_sub(integer_object {3}, integer_object {1}, integer_object {2});
        check_sub(decimal_object {3}, integer_object {1}, decimal_object {2});
        check_sub(integer_object {3}, decimal_object {1}, decimal_object {2});
        check_sub(true_obj, integer_object {1}, integer_object {0});
        check_sub(true_obj, decimal_object {1}, decimal_object {0});
        check_sub(integer_object {2}, true_obj, integer_object {1});
        check_sub(decimal_object {2}, true_obj, decimal_object {1});
        check_sub(false_obj, decimal_object {1}, decimal_object {-1});
        check_sub(false_obj, integer_object {1}, integer_object {-1});
    }

    TEST_CASE("operator *")
    {
        check_mul(integer_object {1}, integer_object {1}, integer_object {1});
        check_mul(decimal_object {2}, integer_object {2}, decimal_object {4});
        check_mul(true_obj, integer_object {2}, integer_object {2});
        check_mul(true_obj, decimal_object {2}, decimal_object {2});
        check_mul(integer_object {2}, true_obj, integer_object {2});
        check_mul(decimal_object {2}, true_obj, decimal_object {2});
        check_mul(false_obj, decimal_object {1}, decimal_object {0});
        check_mul(false_obj, integer_object {1}, integer_object {0});
        check_mul(integer_object {2}, array_obj, array_object {{&int_obj, &int2_obj, &int_obj, &int2_obj}});
        check_mul(array_obj, integer_object {2}, array_object {{&int_obj, &int2_obj, &int_obj, &int2_obj}});
        check_mul(string_object {"abc"}, integer_object {2}, string_object {"abcabc"});
        check_mul(integer_object {2}, string_object {"abc"}, string_object {"abcabc"});
    }

    TEST_CASE("operator /")
    {
        check_div(integer_object {1}, integer_object {1}, decimal_object {1});
        check_div(decimal_object {4}, integer_object {2}, decimal_object {2});
        check_div(true_obj, integer_object {2}, decimal_object {0.5});
        check_div(true_obj, decimal_object {1}, decimal_object {1});
        check_div(integer_object {2}, true_obj, decimal_object {2});
        check_div(decimal_object {2}, true_obj, decimal_object {2});
        check_div(false_obj, decimal_object {1}, decimal_object {0});
        check_div(false_obj, integer_object {1}, decimal_object {0});
        check_div(integer_object {1}, integer_object {0}, error_object {"division by zero"});
        check_div(integer_object {1}, false_obj, error_object {"division by zero"});
        check_inf(integer_object {1} / decimal_object {0});
        check_inf(integer_object {-1} / decimal_object {0});
    }

    TEST_CASE("operator //")
    {
        check_floor_div(integer_object {1}, integer_object {1}, decimal_object {1});
        check_floor_div(decimal_object {5}, integer_object {2}, decimal_object {2});
        check_floor_div(decimal_object {-5}, integer_object {2}, decimal_object {-3});
        check_floor_div(true_obj, integer_object {-2}, decimal_object {-1});
        check_floor_div(true_obj, decimal_object {1}, decimal_object {1});
        check_floor_div(integer_object {2}, true_obj, decimal_object {2});
        check_floor_div(decimal_object {2}, true_obj, decimal_object {2});
        check_floor_div(false_obj, decimal_object {1}, decimal_object {0});
        check_floor_div(false_obj, integer_object {1}, decimal_object {0});
        check_floor_div(integer_object {1}, integer_object {0}, error_object {"division by zero"});
    }
    TEST_CASE("operator %")
    {
        check_mod(integer_object {1}, integer_object {1}, integer_object {0});
        check_mod(integer_object {-1}, integer_object {100}, integer_object {99});
        check_mod(decimal_object {5}, integer_object {2}, decimal_object {1});
        check_mod(decimal_object {-5}, integer_object {2}, decimal_object {1});
        check_mod(true_obj, integer_object {-2}, integer_object {-1});
        check_mod(true_obj, decimal_object {1}, decimal_object {0});
        check_mod(integer_object {2}, true_obj, integer_object {0});
        check_mod(decimal_object {2}, true_obj, decimal_object {0});
        check_mod(false_obj, decimal_object {1}, decimal_object {0});
        check_mod(false_obj, integer_object {1}, integer_object {0});
        check_mod(integer_object {1}, integer_object {0}, error_object {"division by zero"});
        check_mod(integer_object {1}, false_obj, error_object {"division by zero"});
        check_nan(integer_object {1} % decimal_object {0});
    }

    TEST_CASE("operator &")
    {
        check_bit_and(integer_object {1}, integer_object {1}, integer_object {1});
        check_bit_and(integer_object {-1}, integer_object {100}, integer_object {100});
        check_bit_and(integer_object {5}, integer_object {2}, decimal_object {0});
        check_bit_and(integer_object {-5}, integer_object {2}, decimal_object {2});
        check_bit_and(true_obj, integer_object {-2}, integer_object {0});
        check_bit_and(integer_object {2}, true_obj, integer_object {0});
        check_bit_and(false_obj, integer_object {1}, integer_object {0});
    }

    TEST_CASE("operator |")
    {
        check_bit_or(integer_object {1}, integer_object {1}, integer_object {1});
        check_bit_or(integer_object {-1}, integer_object {100}, integer_object {-1});
        check_bit_or(integer_object {5}, integer_object {2}, decimal_object {7});
        check_bit_or(integer_object {-5}, integer_object {2}, decimal_object {-5});
        check_bit_or(true_obj, integer_object {-2}, integer_object {-1});
        check_bit_or(integer_object {2}, true_obj, integer_object {3});
        check_bit_or(false_obj, integer_object {1}, integer_object {1});
    }

    TEST_CASE("operator ^")
    {
        check_bit_xor(integer_object {1}, integer_object {1}, integer_object {0});
        check_bit_xor(integer_object {-1}, integer_object {100}, integer_object {-101});
        check_bit_xor(integer_object {5}, integer_object {2}, decimal_object {7});
        check_bit_xor(integer_object {-5}, integer_object {2}, decimal_object {-7});
        check_bit_xor(true_obj, integer_object {-2}, integer_object {-1});
        check_bit_xor(integer_object {2}, true_obj, integer_object {3});
        check_bit_xor(false_obj, integer_object {1}, integer_object {1});
    }

    TEST_CASE("operator <<")
    {
        check_bit_shl(integer_object {1}, integer_object {1}, integer_object {2});
        check_bit_shl(integer_object {-1}, integer_object {10}, integer_object {-1024});
        check_bit_shl(integer_object {5}, integer_object {2}, decimal_object {20});
        check_bit_shl(integer_object {-5}, integer_object {2}, decimal_object {-20});
        check_bit_shl(true_obj, integer_object {2}, integer_object {4});
        check_bit_shl(integer_object {2}, true_obj, integer_object {4});
        check_bit_shl(false_obj, integer_object {1}, integer_object {0});
    }

    TEST_CASE("operator >>")
    {
        check_bit_shr(integer_object {2}, integer_object {1}, integer_object {1});
        check_bit_shr(integer_object {-10}, integer_object {2}, integer_object {-3});
        check_bit_shr(integer_object {-5}, integer_object {2}, decimal_object {-2});
        check_bit_shr(true_obj, integer_object {1}, integer_object {0});
        check_bit_shr(integer_object {2}, true_obj, integer_object {1});
        check_bit_shr(false_obj, integer_object {1}, integer_object {0});
    }
}

// NOLINTEND(*)
}  // namespace
