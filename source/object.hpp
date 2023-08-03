#pragma once

#include <algorithm>
#include <any>
#include <memory>
#include <stdexcept>
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

struct callable_expression;

struct nullvalue
{
};

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
using return_value = std::any;
using array = std::vector<object_ptr>;
using hash = std::unordered_map<object_ptr, object_ptr, object_hash>;
using bound_function = std::pair<const callable_expression*, environment_ptr>;

using value_type =
    std::variant<nullvalue, bool, integer_value, string_value, error, array, hash, bound_function, return_value>;

namespace std
{
auto to_string(const value_type&) -> std::string;
}  // namespace std
struct object
{
    object() = default;
    object(const object&) = default;
    template<typename T>
    inline constexpr auto is() const -> bool
    {
        return std::holds_alternative<T>(value);
    }

    inline constexpr auto is_nil() const -> bool { return is<nullvalue>(); }

    template<typename T>
    auto as() const -> const T&
    {
        if (!is<T>()) {
            throw std::runtime_error("Error trying to convert " + std::to_string(value) + " to "
                                     + object {T {}}.type_name());
        }
        return std::get<T>(value);
    }
    inline auto to_ptr() const -> object_ptr { return std::make_shared<object>(this->value); }
    value_type value {};
    auto type_name() const -> std::string;

    inline object(value_type val)
        : value {std::move(val)}
    {
    }
};

auto unwrap_return_value(const object& obj) -> object;

template<typename... T>
auto make_error(fmt::format_string<T...> fmt, T&&... args) -> object
{
    return object(error {.message = fmt::format(fmt, std::forward<T>(args)...)});
}

inline constexpr auto operator==(const object& lhs, const object& rhs) -> bool
{
    return std::visit(overloaded {[](const nullvalue, const nullvalue) { return true; },
                                  [](const bool val1, const bool val2) { return val1 == val2; },
                                  [](const integer_value val1, const integer_value val2) { return val1 == val2; },
                                  [](const string_value& val1, const string_value& val2) { return val1 == val2; },
                                  [](const bound_function& /*val1*/, const bound_function& /*val2*/) { return false; },
                                  [](const array& arr1, const array& arr2)
                                  {
                                      return arr1.size() == arr2.size()
                                          && std::equal(arr1.cbegin(),
                                                        arr1.cend(),
                                                        arr2.begin(),
                                                        [](const object_ptr& lhs, const object_ptr& rhs)
                                                        { return *lhs == *rhs; });
                                  },
                                  [](const auto&, const auto&) { return false; }},
                      lhs.value,
                      rhs.value);
}
