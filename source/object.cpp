#include <string>

#include "object.hpp"

namespace std
{
auto to_string(const value_type& value) -> std::string
{
    return std::visit(overloaded {[](const nullvalue&) -> std::string { return "nil"; },
                                  [](const integer_value val) -> std::string { return to_string(val); },
                                  [](const string_value& val) -> std::string { return "\"" + val + "\""; },
                                  [](const bool val) -> std::string { return val ? "true" : "false"; },
                                  [](const error& val) -> std::string { return "ERROR: " + val.message; },
                                  [](const auto&) -> std::string { return "unknown"; }},
                      value);
}
}  // namespace std

auto object::type_name() const -> std::string
{
    return std::visit(
        overloaded {
            [](const nullvalue&) { return "nil"; },
            [](const bool) { return "bool"; },
            [](const integer_value) { return "Integer"; },
            [](const string_value&) { return "String"; },
            [](const error&) { return "Error"; },
            [](const auto&) { return "unknown"; },
        },
        value);
}
