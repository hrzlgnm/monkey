#pragma once

#include <algorithm>
#include <any>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include <fmt/core.h>

#include "environment_fwd.hpp"

// helper type for std::visit
template<typename... T>
struct overloaded : T...
{
    using T::operator()...;
};
template<class... T>
overloaded(T...) -> overloaded<T...>;

using nil_type = std::monostate;

struct error
{
    std::string message;
};

struct object;
using object_ptr = std::shared_ptr<object>;
struct object_hash
{
    auto operator()(const object_ptr& obj) const -> size_t;
};

using integer_value = std::int64_t;
using string_value = std::string;
using array = std::vector<object>;
using hash_key_type = std::variant<integer_value, string_value, bool>;
using hash = std::unordered_map<hash_key_type, std::any>;

struct callable_expression;
using bound_function = std::pair<const callable_expression*, environment_ptr>;

using value_type = std::variant<nil_type, bool, integer_value, string_value, error, array, hash, bound_function>;

namespace std
{
auto to_string(const value_type&) -> std::string;
}  // namespace std

struct object
{
    template<typename T>
    inline constexpr auto is() const -> bool
    {
        return std::holds_alternative<T>(value);
    }
    inline constexpr auto is_nil() const -> bool { return is<nil_type>(); }
    inline constexpr auto is_hashable() const -> bool
    {
        return is<integer_value>() || is<string_value>() || is<bool>();
    }

    inline auto hash_key() const -> hash_key_type
    {
        return std::visit(overloaded {[](const integer_value integer) -> hash_key_type { return {integer}; },
                                      [](const string_value& str) -> hash_key_type { return {str}; },
                                      [](const bool val) -> hash_key_type { return {val}; },
                                      [](const auto& other) -> hash_key_type
                                      { throw std::invalid_argument(object {other}.type_name() + "is not hashable"); }},
                          value);
    }

    template<typename T>
    auto as() const -> const T&
    {
        if (!is<T>()) {
            throw std::runtime_error("Error trying to convert " + std::to_string(value) + " to "
                                     + object {T {}}.type_name());
        }
        return std::get<T>(value);
    }
    value_type value {};
    bool is_return_value {};
    auto type_name() const -> std::string;
};

static const nil_type nil {};
auto unwrap(const std::any& obj) -> object;

template<typename... T>
auto make_error(fmt::format_string<T...> fmt, T&&... args) -> object
{
    return object(error {.message = fmt::format(fmt, std::forward<T>(args)...)});
}

inline constexpr auto operator==(const object& lhs, const object& rhs) -> bool
{
    return std::visit(
        overloaded {[](const nil_type, const nil_type) { return true; },
                    [](const bool val1, const bool val2) { return val1 == val2; },
                    [](const integer_value val1, const integer_value val2) { return val1 == val2; },
                    [](const string_value& val1, const string_value& val2) { return val1 == val2; },
                    [](const bound_function& /*val1*/, const bound_function& /*val2*/) { return false; },
                    [](const array& arr1, const array& arr2)
                    { return arr1.size() == arr2.size() && std::equal(arr1.cbegin(), arr1.cend(), arr2.begin()); },
                    [](const auto&, const auto&) { return false; }},
        lhs.value,
        rhs.value);
}
