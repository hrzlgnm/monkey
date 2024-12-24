#include <algorithm>
#include <cmath>
#include <cstdint>
#include <ios>
#include <iterator>
#include <ostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

#include "object.hpp"

#include <ast/builtin_function_expression.hpp>
#include <ast/callable_expression.hpp>
#include <code/code.hpp>
#include <doctest/doctest.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <gc.hpp>
#include <overloaded.hpp>

using enum object::object_type;

namespace
{
const boolean_object false_obj {/*val=*/false};
const boolean_object true_obj {/*val=*/true};
const struct null_object null_obj;
const struct break_object break_obj;
const struct continue_object continue_obj;

auto invert_boolean_object(const object* b) -> const object*
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

template<typename T>
auto eq_helper(const T* t, const object& other) -> const object*
{
    if constexpr (std::is_same_v<T, decimal_object>) {
        return native_bool_to_object(other.is(t->type()) && are_almost_equal(t->value, other.as<T>()->value));
    }
    return native_bool_to_object(other.is(t->type()) && t->value == other.as<T>()->value);
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
        return &true_obj;
    }
    return &false_obj;
}

auto tru() -> const object*
{
    return &true_obj;
}

auto fals() -> const object*
{
    return &false_obj;
}

auto null() -> const object*
{
    return &null_obj;
}

auto brake() -> const object*
{
    return &break_obj;
}

auto cont() -> const object*
{
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
        case nll:
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
        case object::object_type::return_value:
            return ostrm << "return_value";
        case object::object_type::brek:
            return ostrm << "break";
        case object::object_type::cntn:
            return ostrm << "continue";
    }
    return ostrm << "unknown " << static_cast<int>(type);
}

auto string_object::hash_key() const -> hash_key_type
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
        return multiply_sequence_helper(this, other.as<integer_object>()->value);
    }
    return nullptr;
}

auto boolean_object::hash_key() const -> hash_key_type
{
    return value;
}

auto boolean_object::cast_to(object_type type) const -> const object*
{
    switch (type) {
        case boolean:
            return this;
        case decimal:
            return make<decimal_object>(static_cast<decimal_object::value_type>(value));
        case integer:
            return make<integer_object>(static_cast<integer_object::value_type>(value));
        default:
            break;
    }
    return nullptr;
}

auto boolean_object::operator==(const object& other) const -> const object*
{
    if (other.is(decimal)) {
        return eq_helper(cast_to(decimal)->as<decimal_object>(), other);
    }
    if (other.is(integer)) {
        return eq_helper(cast_to(integer)->as<integer_object>(), other);
    }
    return eq_helper(this, other);
}

auto boolean_object::operator>(const object& other) const -> const object*
{
    if (other.is(integer)) {
        return gt_helper(cast_to(integer)->as<integer_object>(), other);
    }
    if (other.is(decimal)) {
        return gt_helper(cast_to(decimal)->as<decimal_object>(), other);
    }
    return gt_helper(this, other);
}

auto boolean_object::operator+(const object& other) const -> const object*
{
    if (other.is(integer)) {
        return other + *cast_to(integer);
    }
    if (other.is(decimal)) {
        return other + *cast_to(decimal);
    }
    if (other.is(boolean)) {
        return *(cast_to(integer)) + *other.cast_to(integer);
    }
    return nullptr;
}

auto boolean_object::operator-(const object& other) const -> const object*
{
    if (other.is(integer)) {
        return *cast_to(integer) - other;
    }
    if (other.is(decimal)) {
        return *cast_to(decimal) - other;
    }
    if (other.is(boolean)) {
        return *(cast_to(integer)) + *other.cast_to(integer);
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
        return *(cast_to(integer)) * *other.cast_to(integer);
    }
    return nullptr;
}

auto boolean_object::operator/(const object& other) const -> const object*
{
    if (other.is(decimal) || other.is(integer) || other.is(boolean)) {
        return *cast_to(decimal) / other;
    }
    return nullptr;
}

auto boolean_object::operator%(const object& other) const -> const object*
{
    if (other.is(boolean) || other.is(integer)) {
        return *(cast_to(integer)) % *other.cast_to(integer);
    }
    if (other.is(decimal)) {
        return *cast_to(decimal) % other;
    }
    return nullptr;
}

auto boolean_object::operator&(const object& other) const -> const object*
{
    if (other.is(boolean)) {
        return make<boolean_object>(
            ((static_cast<uint8_t>(value) & static_cast<uint8_t>(other.as<boolean_object>()->value)) != 0));
    }
    if (other.is(integer)) {
        return *cast_to(integer) & other;
    }
    return nullptr;
}

auto boolean_object::operator|(const object& other) const -> const object*
{
    if (other.is(boolean)) {
        return make<boolean_object>(
            ((static_cast<uint8_t>(value) | static_cast<uint8_t>(other.as<boolean_object>()->value)) != 0));
    }
    if (other.is(integer)) {
        return *cast_to(integer) | other;
    }
    return nullptr;
}

auto boolean_object::operator^(const object& other) const -> const object*
{
    if (other.is(boolean)) {
        return make<boolean_object>(
            ((static_cast<uint8_t>(value) ^ static_cast<uint8_t>(other.as<boolean_object>()->value)) != 0));
    }
    if (other.is(integer)) {
        return *cast_to(integer) ^ other;
    }
    return nullptr;
}

auto boolean_object::operator<<(const object& other) const -> const object*
{
    if (other.is(boolean)) {
        return make<integer_object>(static_cast<uint8_t>(value)
                                    << static_cast<uint8_t>(other.as<boolean_object>()->value));
    }
    if (other.is(integer)) {
        return *cast_to(integer) << other;
    }
    return nullptr;
}

auto boolean_object::operator>>(const object& other) const -> const object*
{
    if (other.is(boolean)) {
        return make<integer_object>(static_cast<uint8_t>(value)
                                    >> static_cast<uint8_t>(other.as<boolean_object>()->value));
    }
    if (other.is(integer)) {
        return *cast_to(integer) >> other;
    }
    return nullptr;
}

auto integer_object::hash_key() const -> hash_key_type
{
    return value;
}

auto integer_object::cast_to(object_type type) const -> const object*
{
    switch (type) {
        case integer:
            return this;
        case decimal:
            return make<decimal_object>(static_cast<decimal_object::value_type>(value));
        default:
            break;
    }
    return nullptr;
}

auto integer_object::operator==(const object& other) const -> const object*
{
    if (const auto* const other_as_int = other.cast_to(integer); other_as_int != nullptr) {
        return eq_helper(this, *other_as_int);
    }
    if (other.is(decimal)) {
        if (const auto* other_as_decimal = other.cast_to(decimal); other_as_decimal != nullptr) {
            return eq_helper(cast_to(decimal)->as<decimal_object>(), *other_as_decimal);
        }
    }
    return object::operator==(other);
}

auto integer_object::operator>(const object& other) const -> const object*
{
    if (other.is(decimal)) {
        return gt_helper(cast_to(decimal)->as<decimal_object>(), other);
    }
    if (const auto* const other_as_int = other.cast_to(integer); other_as_int != nullptr) {
        return gt_helper(this, *other_as_int);
    }
    return nullptr;
}

auto integer_object::operator+(const object& other) const -> const object*
{
    if (other.is(integer) || other.is(boolean)) {
        return make<integer_object>(value + other.cast_to(integer)->as<integer_object>()->value);
    }
    if (other.is(decimal)) {
        return other + *cast_to(decimal);
    }
    return nullptr;
}

auto integer_object::operator-(const object& other) const -> const object*
{
    if (other.is(integer) || other.is(boolean)) {
        return make<integer_object>(value - other.cast_to(integer)->as<integer_object>()->value);
    }
    if (other.is(decimal)) {
        return *cast_to(decimal) - other;
    }
    return nullptr;
}

auto integer_object::operator*(const object& other) const -> const object*
{
    if (other.is(integer) || other.is(boolean)) {
        return make<integer_object>(value * other.cast_to(integer)->as<integer_object>()->value);
    }
    if (other.is(decimal)) {
        return *cast_to(decimal) * other;
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
    if (other.is(integer) || other.is(boolean)) {
        const auto other_value = other.cast_to(integer)->as<integer_object>()->value;
        if (other_value == 0) {
            return make_error("division by zero");
        }
        return make<decimal_object>(static_cast<decimal_object::value_type>(value)
                                    / static_cast<decimal_object::value_type>(other_value));
    }
    if (other.is(decimal)) {
        return *cast_to(decimal) / other;
    }
    return nullptr;
}

auto integer_object::operator%(const object& other) const -> const object*
{
    const auto* other_as_int = other.cast_to(integer);
    if (other_as_int != nullptr) {
        if (other_as_int->as<integer_object>()->value == 0) {
            return make_error("division by zero");
        }
        return make<integer_object>(math_mod(value, other_as_int->as<integer_object>()->value));
    }
    if (other.is(decimal)) {
        return *cast_to(decimal) % other;
    }
    return nullptr;
}

auto integer_object::operator|(const object& other) const -> const object*
{
    const auto* other_as_int = other.cast_to(integer);
    if (other_as_int != nullptr) {
        // todo cast to uint64_t?
        return make<integer_object>(value | other_as_int->as<integer_object>()->value);
    }
    return nullptr;
}

auto integer_object::operator^(const object& other) const -> const object*
{
    const auto* other_as_int = other.cast_to(integer);
    if (other_as_int != nullptr) {
        // todo cast to uint64_t?
        return make<integer_object>(value ^ other_as_int->as<integer_object>()->value);
    }
    return nullptr;
}

auto integer_object::operator<<(const object& other) const -> const object*
{
    const auto* other_as_int = other.cast_to(integer);
    if (other_as_int != nullptr) {
        // todo cast to uint64_t?
        return make<integer_object>(value << other_as_int->as<integer_object>()->value);
    }
    return nullptr;
}

auto integer_object::operator>>(const object& other) const -> const object*
{
    const auto* other_as_int = other.cast_to(integer);
    if (other_as_int != nullptr) {
        // todo cast to uint64_t?
        return make<integer_object>(value >> other_as_int->as<integer_object>()->value);
    }
    return nullptr;
}

auto integer_object::operator&(const object& other) const -> const object*
{
    const auto* other_as_int = other.cast_to(integer);
    if (other_as_int != nullptr) {
        // todo cast to uint64_t?
        return make<integer_object>(value & other_as_int->as<integer_object>()->value);
    }
    return nullptr;
}

auto decimal_object::cast_to(object_type type) const -> const object*
{
    if (type == decimal) {
        return this;
    }
    return nullptr;
}

auto decimal_object::operator==(const object& other) const -> const object*
{
    if (const auto* const other_as_decimal = other.cast_to(decimal); other_as_decimal != nullptr) {
        return eq_helper(this, *other_as_decimal);
    }
    return object::operator==(other);
}

auto decimal_object::operator+(const object& other) const -> const object*
{
    const auto* other_as_decimal = other.cast_to(decimal);
    if (other_as_decimal != nullptr) {
        return make<decimal_object>(value + other_as_decimal->as<decimal_object>()->value);
    }
    return nullptr;
}

auto decimal_object::operator-(const object& other) const -> const object*
{
    const auto* other_as_decimal = other.cast_to(decimal);
    if (other_as_decimal != nullptr) {
        return make<decimal_object>(value - other_as_decimal->as<decimal_object>()->value);
    }
    return nullptr;
}

auto decimal_object::operator*(const object& other) const -> const object*
{
    const auto* other_as_decimal = other.cast_to(decimal);
    if (other_as_decimal != nullptr) {
        return make<decimal_object>(value * other_as_decimal->as<decimal_object>()->value);
    }
    return nullptr;
}

auto decimal_object::operator/(const object& other) const -> const object*
{
    const auto* other_as_decimal = other.cast_to(decimal);
    if (other_as_decimal != nullptr) {
        return make<decimal_object>(value / other_as_decimal->as<decimal_object>()->value);
    }
    return nullptr;
}

auto decimal_object::operator%(const object& other) const -> const object*
{
    const auto* other_as_decimal = other.cast_to(decimal);
    if (other_as_decimal != nullptr) {
        return make<decimal_object>(math_mod(value, other_as_decimal->as<decimal_object>()->value));
    }
    return nullptr;
}

auto decimal_object::operator>(const object& other) const -> const object*
{
    const auto* other_as_decimal = other.cast_to(decimal);
    if (other_as_decimal != nullptr) {
        return gt_helper(this, *other_as_decimal);
    }
    return nullptr;
}

auto object_floor_div(const object* lhs, const object* rhs) -> const object*
{
    const auto* div = (*lhs / *rhs);
    if (div->is(object::object_type::decimal)) {
        return make<decimal_object>(std::floor(div->as<decimal_object>()->value));
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
    return callable->string();
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
        return multiply_sequence_helper(this, other.as<integer_object>()->value);
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
    return fmt::format("{{\n{}}}", to_string(instrs));
}

[[nodiscard]] auto closure_object::clone() const -> closure_object*
{
    return make<closure_object>(fn, free);
}

auto closure_object::inspect() const -> std::string
{
    std::ostringstream strm;
    strm << "[";
    for (bool first = true; const auto* const element : free) {
        if (!first) {
            strm << ", ";
        }
        strm << fmt::format("{}", element->inspect());
        first = false;
    }
    strm << "]";

    return fmt::format("fn<closure>() compiled: {}\n free: {}", fn->inspect(), strm.str());
}

namespace
{
// NOLINTBEGIN(*)

auto op_defined(const object* res, const std::string_view& op, const object& lhs, const object& rhs) -> bool
{
    if (res == nullptr) {
        INFO("operator ", lhs.type(), " == ", rhs.type(), " is not defined ");
        REQUIRE(res != nullptr);
        return false;
    }
    return true;
}

auto require_eq(const object& lhs, const object& rhs) -> void
{
    const auto* res = lhs == rhs;
    if (!op_defined(res, "==", lhs, rhs)) {
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
    REQUIRE((*res == *tru()) == tru());
}

auto require_ne(const object& lhs, const object& rhs) -> void
{
    const auto* res = lhs != rhs;
    if (!op_defined(res, "!=", lhs, rhs)) {
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
    REQUIRE((*res == *tru()) == tru());
}

auto require_add(const object& lhs, const object& rhs, const object& expected) -> void
{
    const auto* res = lhs + rhs;
    if (!op_defined(res, "+", lhs, rhs)) {
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
    REQUIRE((*res == expected) == tru());
}

auto require_sub(const object& lhs, const object& rhs, const object& expected) -> void
{
    const auto* res = lhs - rhs;
    if (!op_defined(res, "-", lhs, rhs)) {
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
    REQUIRE((*res == expected) == tru());
}

auto require_mul(const object& lhs, const object& rhs, const object& expected) -> void
{
    const auto* res = lhs * rhs;
    if (!op_defined(res, "*", lhs, rhs)) {
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
    REQUIRE((*res == expected) == tru());
}

auto require_nan(const object* expected) -> void
{
    REQUIRE(expected != nullptr);
    REQUIRE(expected->is(decimal));
    REQUIRE(std::isnan(expected->as<decimal_object>()->value));
}

auto require_inf(const object* expected) -> void
{
    REQUIRE(expected != nullptr);
    REQUIRE(expected->is(decimal));
    REQUIRE(std::isinf(expected->as<decimal_object>()->value));
}

auto require_div(const object& lhs, const object& rhs, const object& expected) -> void
{
    const auto* res = lhs / rhs;
    if (!op_defined(res, "/", lhs, rhs)) {
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
    REQUIRE((*res == expected) == tru());
}

auto require_floor_div(const object& lhs, const object& rhs, const object& expected) -> void
{
    const auto* res = object_floor_div(&lhs, &rhs);
    if (!op_defined(res, "//", lhs, rhs)) {
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
    REQUIRE((*res == expected) == tru());
}

auto require_mod(const object& lhs, const object& rhs, const object& expected) -> void
{
    const auto* res = lhs % rhs;
    if (!op_defined(res, "%", lhs, rhs)) {
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
    REQUIRE((*res == expected) == tru());
}

auto require_bit_and(const object& lhs, const object& rhs, const object& expected) -> void
{
    const auto* res = lhs & rhs;
    if (!op_defined(res, "&", lhs, rhs)) {
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
    REQUIRE((*res == expected) == tru());
}

auto require_bit_or(const object& lhs, const object& rhs, const object& expected) -> void
{
    const auto* res = lhs | rhs;
    if (!op_defined(res, "|", lhs, rhs)) {
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
    REQUIRE((*res == expected) == tru());
}

auto require_bit_xor(const object& lhs, const object& rhs, const object& expected) -> void
{
    const auto* res = lhs ^ rhs;
    if (!op_defined(res, "^", lhs, rhs)) {
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
    REQUIRE((*res == expected) == tru());
}

auto require_bit_shl(const object& lhs, const object& rhs, const object& expected) -> void
{
    const auto* res = lhs << rhs;
    if (!op_defined(res, "<<", lhs, rhs)) {
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
    REQUIRE((*res == expected) == tru());
}

auto require_bit_shr(const object& lhs, const object& rhs, const object& expected) -> void
{
    const auto* res = lhs >> rhs;
    if (!op_defined(res, ">", lhs, rhs)) {
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
    REQUIRE((*res == expected) == tru());
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
    const error_object err_obj {"error"};
    const integer_object int2_obj {i2};
    const array_object array_obj {{&int_obj, &int2_obj}};
    const hash_object hash_obj {{{1, &str_obj}, {2, &true_obj}}};
    const return_value_object ret_obj {&array_obj};
    const function_object function_obj {nullptr, nullptr};
    const builtin_object builtin_obj {builtin_function_expression::builtins()[0]};
    const compiled_function_object cmpld_obj {{}, 0, 0};
    const closure_object clsr_obj {&cmpld_obj, {}};

    TEST_CASE("is truthy")
    {
        REQUIRE_FALSE(integer_object {0}.is_truthy());
        REQUIRE_FALSE(string_object {""}.is_truthy());
        REQUIRE_FALSE(false_obj.is_truthy());
        REQUIRE_FALSE(null()->is_truthy());
        REQUIRE_FALSE(array_object {{}}.is_truthy());
        REQUIRE_FALSE(hash_object {{}}.is_truthy());
        REQUIRE_FALSE(decimal_object {0}.is_truthy());
        REQUIRE(integer_object {1}.is_truthy());
        REQUIRE(decimal_object {1}.is_truthy());
        REQUIRE(string_object {"1"}.is_truthy());
        REQUIRE(true_obj.is_truthy());
        REQUIRE(array_object {{&int_obj}}.is_truthy());
        REQUIRE(integer_object {1}.is_truthy());
        REQUIRE(integer_object {1}.is_truthy());
        REQUIRE(error_object {{}}.is_truthy());
        REQUIRE(return_value_object {tru()}.is_truthy());
        REQUIRE(function_obj.is_truthy());
        REQUIRE(builtin_obj.is_truthy());
        REQUIRE(cmpld_obj.is_truthy());
        REQUIRE(clsr_obj.is_truthy());
    }
    TEST_CASE("type")
    {
        using enum object::object_type;
        REQUIRE_EQ(int_obj.type(), integer);
        REQUIRE_EQ(dec_obj.type(), decimal);
        REQUIRE_EQ(true_obj.type(), boolean);
        REQUIRE_EQ(str_obj.type(), string);
        REQUIRE_EQ(brake()->type(), brek);
        REQUIRE_EQ(cont()->type(), cntn);
        REQUIRE_EQ(null()->type(), nll);
        REQUIRE_EQ(err_obj.type(), error);
        REQUIRE_EQ(array_obj.type(), array);
        REQUIRE_EQ(hash_obj.type(), hash);
        REQUIRE_EQ(return_value_object {&int_obj}.type(), return_value);
        REQUIRE_EQ(function_obj.type(), function);
        REQUIRE_EQ(cmpld_obj.type(), compiled_function);
        REQUIRE_EQ(clsr_obj.type(), closure);
        REQUIRE_EQ(builtin_obj.type(), builtin);
    }
    TEST_CASE("inspect")
    {
        REQUIRE_EQ(int_obj.inspect(), "123");
        REQUIRE_EQ(dec_obj.inspect(), "12.3");
        REQUIRE_EQ(true_obj.inspect(), "true");
        REQUIRE_EQ(str_obj.inspect(), R"("str")");
        REQUIRE_EQ(err_obj.inspect(), "ERROR: error");
        REQUIRE_EQ(brake()->inspect(), "break");
        REQUIRE_EQ(cont()->inspect(), "continue");
        REQUIRE_EQ(null()->inspect(), "null");
        REQUIRE_EQ(array_obj.inspect(), "[123, 124]");
        REQUIRE_EQ(hash_object {{{1, &str_obj}}}.inspect(), R"({1: "str"})");
        REQUIRE_EQ(ret_obj.inspect(), R"([123, 124])");
    }
    TEST_CASE("operator ==")
    {
        require_eq(int_obj, integer_object {i1});
        require_eq(dec_obj, decimal_object {d1});
        require_eq(decimal_object {d3}, integer_object {i1});
        require_eq(true_obj, boolean_object {true});
        require_eq(true_obj, integer_object {1});
        require_eq(true_obj, decimal_object {1});
        require_eq(integer_object {1}, true_obj);
        require_eq(decimal_object {1}, true_obj);
        require_eq(false_obj, boolean_object {false});
        require_eq(null_obj, null_object {});
        require_eq(array_obj, array_object {{&int_obj, &int2_obj}});
        require_eq(hash_obj, hash_object {{{2, &true_obj}, {1, &str_obj}}});
    }

    TEST_CASE("operator !=")
    {
        require_ne(int_obj, integer_object {122});
        require_ne(decimal_object {d2}, integer_object {i1});
        require_ne(true_obj, false_obj);
        require_ne(false_obj, null_obj);
        require_ne(null_obj, true_obj);
        require_ne(hash_obj, hash_object {{{2, &str_obj}, {1, &int_obj}}});
        require_ne(array_obj, array_object {{&int2_obj, &int_obj}});
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
        require_add(integer_object {1}, integer_object {1}, integer_object {2});
        require_add(decimal_object {1}, integer_object {1}, decimal_object {2});
        require_add(true_obj, integer_object {1}, integer_object {2});
        require_add(true_obj, decimal_object {1}, decimal_object {2});
        require_add(integer_object {1}, true_obj, integer_object {2});
        require_add(decimal_object {1}, true_obj, decimal_object {2});
        require_add(false_obj, decimal_object {1}, decimal_object {1});
        require_add(false_obj, integer_object {1}, integer_object {1});
        require_add(array_obj, array_obj, array_object {{&int_obj, &int2_obj, &int_obj, &int2_obj}});
        require_add(
            hash_obj, hash_object {{{3, &false_obj}}}, hash_object {{{1, &str_obj}, {2, &true_obj}, {3, &false_obj}}});
    }
    TEST_CASE("operator -")
    {
        require_sub(integer_object {3}, integer_object {1}, integer_object {2});
        require_sub(decimal_object {3}, integer_object {1}, decimal_object {2});
        require_sub(integer_object {3}, decimal_object {1}, decimal_object {2});
        require_sub(true_obj, integer_object {1}, integer_object {0});
        require_sub(true_obj, decimal_object {1}, decimal_object {0});
        require_sub(integer_object {2}, true_obj, integer_object {1});
        require_sub(decimal_object {2}, true_obj, decimal_object {1});
        require_sub(false_obj, decimal_object {1}, decimal_object {-1});
        require_sub(false_obj, integer_object {1}, integer_object {-1});
    }

    TEST_CASE("operator *")
    {
        require_mul(integer_object {1}, integer_object {1}, integer_object {1});
        require_mul(decimal_object {2}, integer_object {2}, decimal_object {4});
        require_mul(true_obj, integer_object {2}, integer_object {2});
        require_mul(true_obj, decimal_object {2}, decimal_object {2});
        require_mul(integer_object {2}, true_obj, integer_object {2});
        require_mul(decimal_object {2}, true_obj, decimal_object {2});
        require_mul(false_obj, decimal_object {1}, decimal_object {0});
        require_mul(false_obj, integer_object {1}, integer_object {0});
        require_mul(integer_object {2}, array_obj, array_object {{&int_obj, &int2_obj, &int_obj, &int2_obj}});
        require_mul(array_obj, integer_object {2}, array_object {{&int_obj, &int2_obj, &int_obj, &int2_obj}});
        require_mul(string_object {"abc"}, integer_object {2}, string_object {"abcabc"});
        require_mul(integer_object {2}, string_object {"abc"}, string_object {"abcabc"});
    }

    TEST_CASE("operator /")
    {
        require_div(integer_object {1}, integer_object {1}, decimal_object {1});
        require_div(decimal_object {4}, integer_object {2}, decimal_object {2});
        require_div(true_obj, integer_object {2}, decimal_object {0.5});
        require_div(true_obj, decimal_object {1}, decimal_object {1});
        require_div(integer_object {2}, true_obj, decimal_object {2});
        require_div(decimal_object {2}, true_obj, decimal_object {2});
        require_div(false_obj, decimal_object {1}, decimal_object {0});
        require_div(false_obj, integer_object {1}, decimal_object {0});
        require_div(integer_object {1}, integer_object {0}, error_object {"division by zero"});
        require_div(integer_object {1}, false_obj, error_object {"division by zero"});
        require_inf(integer_object {1} / decimal_object {0});
        require_inf(integer_object {-1} / decimal_object {0});
    }

    TEST_CASE("operator //")
    {
        require_floor_div(integer_object {1}, integer_object {1}, decimal_object {1});
        require_floor_div(decimal_object {5}, integer_object {2}, decimal_object {2});
        require_floor_div(decimal_object {-5}, integer_object {2}, decimal_object {-3});
        require_floor_div(true_obj, integer_object {-2}, decimal_object {-1});
        require_floor_div(true_obj, decimal_object {1}, decimal_object {1});
        require_floor_div(integer_object {2}, true_obj, decimal_object {2});
        require_floor_div(decimal_object {2}, true_obj, decimal_object {2});
        require_floor_div(false_obj, decimal_object {1}, decimal_object {0});
        require_floor_div(false_obj, integer_object {1}, decimal_object {0});
        require_floor_div(integer_object {1}, integer_object {0}, error_object {"division by zero"});
    }
    TEST_CASE("operator %")
    {
        require_mod(integer_object {1}, integer_object {1}, integer_object {0});
        require_mod(integer_object {-1}, integer_object {100}, integer_object {99});
        require_mod(decimal_object {5}, integer_object {2}, decimal_object {1});
        require_mod(decimal_object {-5}, integer_object {2}, decimal_object {1});
        require_mod(true_obj, integer_object {-2}, integer_object {-1});
        require_mod(true_obj, decimal_object {1}, decimal_object {0});
        require_mod(integer_object {2}, true_obj, integer_object {0});
        require_mod(decimal_object {2}, true_obj, decimal_object {0});
        require_mod(false_obj, decimal_object {1}, decimal_object {0});
        require_mod(false_obj, integer_object {1}, integer_object {0});
        require_mod(integer_object {1}, integer_object {0}, error_object {"division by zero"});
        require_mod(integer_object {1}, false_obj, error_object {"division by zero"});
        require_nan(integer_object {1} % decimal_object {0});
    }

    TEST_CASE("operator &")
    {
        require_bit_and(integer_object {1}, integer_object {1}, integer_object {1});
        require_bit_and(integer_object {-1}, integer_object {100}, integer_object {100});
        require_bit_and(integer_object {5}, integer_object {2}, decimal_object {0});
        require_bit_and(integer_object {-5}, integer_object {2}, decimal_object {2});
        require_bit_and(true_obj, integer_object {-2}, integer_object {0});
        require_bit_and(integer_object {2}, true_obj, integer_object {0});
        require_bit_and(false_obj, integer_object {1}, integer_object {0});
    }

    TEST_CASE("operator |")
    {
        require_bit_or(integer_object {1}, integer_object {1}, integer_object {1});
        require_bit_or(integer_object {-1}, integer_object {100}, integer_object {-1});
        require_bit_or(integer_object {5}, integer_object {2}, decimal_object {7});
        require_bit_or(integer_object {-5}, integer_object {2}, decimal_object {-5});
        require_bit_or(true_obj, integer_object {-2}, integer_object {-1});
        require_bit_or(integer_object {2}, true_obj, integer_object {3});
        require_bit_or(false_obj, integer_object {1}, integer_object {1});
    }

    TEST_CASE("operator ^")
    {
        require_bit_xor(integer_object {1}, integer_object {1}, integer_object {0});
        require_bit_xor(integer_object {-1}, integer_object {100}, integer_object {-101});
        require_bit_xor(integer_object {5}, integer_object {2}, decimal_object {7});
        require_bit_xor(integer_object {-5}, integer_object {2}, decimal_object {-7});
        require_bit_xor(true_obj, integer_object {-2}, integer_object {-1});
        require_bit_xor(integer_object {2}, true_obj, integer_object {3});
        require_bit_xor(false_obj, integer_object {1}, integer_object {1});
    }

    TEST_CASE("operator <<")
    {
        require_bit_shl(integer_object {1}, integer_object {1}, integer_object {2});
        require_bit_shl(integer_object {-1}, integer_object {10}, integer_object {-1024});
        require_bit_shl(integer_object {5}, integer_object {2}, decimal_object {20});
        require_bit_shl(integer_object {-5}, integer_object {2}, decimal_object {-20});
        require_bit_shl(true_obj, integer_object {2}, integer_object {4});
        require_bit_shl(integer_object {2}, true_obj, integer_object {4});
        require_bit_shl(false_obj, integer_object {1}, integer_object {0});
    }

    TEST_CASE("operator >>")
    {
        require_bit_shr(integer_object {2}, integer_object {1}, integer_object {1});
        require_bit_shr(integer_object {-10}, integer_object {2}, integer_object {-3});
        require_bit_shr(integer_object {-5}, integer_object {2}, decimal_object {-2});
        require_bit_shr(true_obj, integer_object {1}, integer_object {0});
        require_bit_shr(integer_object {2}, true_obj, integer_object {1});
        require_bit_shr(false_obj, integer_object {1}, integer_object {0});
    }
}

// NOLINTEND(*)
}  // namespace
