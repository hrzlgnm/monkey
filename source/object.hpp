#pragma once

#include <any>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <variant>

// helper type for std::visit
template<typename... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

struct nullvalue
{
};
struct error
{
    std::string message;
};
using integer_value = std::int64_t;
using string_value = std::string;
using return_value = std::any;
using value_type = std::variant<nullvalue, bool, integer_value, string_value, return_value, error>;

namespace std
{
auto to_string(const value_type&) -> std::string;
}  // namespace std

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
    value_type value;
    auto type_name() const -> std::string;
};

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
