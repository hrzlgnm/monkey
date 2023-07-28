#pragma once

#include <stdexcept>
#include <utility>

#include <fmt/core.h>

#include "value_type.hpp"

struct object
{
    template<typename T>
    constexpr auto is() const -> bool
    {
        return std::holds_alternative<T>(value);
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
    auto type_name() const -> std::string;
};

template<typename... T>
auto make_error(fmt::format_string<T...> fmt, T&&... args) -> object
{
    return {error {.message = fmt::format(fmt, std::forward<T>(args)...)}};
}

inline constexpr auto operator==(const object& lhs, const object& rhs) -> bool
{
    return std::visit(overloaded {[](const nullvalue, const nullvalue) { return true; },
                                  [](const bool val1, const bool val2) { return val1 == val2; },
                                  [](const integer_value val1, const integer_value val2) { return val1 == val2; },
                                  [](const string_value& val1, const string_value& val2) { return val1 == val2; },
                                  [](const auto&, const auto&) { return false; }},
                      lhs.value,
                      rhs.value);
}