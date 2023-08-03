#include <algorithm>
#include <iterator>
#include <string>

#include "object.hpp"

#include "callable_expression.hpp"

namespace std
{
auto to_string(const value_type& value) -> std::string
{
    return std::visit(overloaded {[](const nullvalue&) -> std::string { return "nil"; },
                                  [](const integer_value val) -> std::string { return to_string(val); },
                                  [](const string_value& val) -> std::string { return "\"" + val + "\""; },
                                  [](const bool val) -> std::string { return val ? "true" : "false"; },
                                  [](const error& val) -> std::string { return "ERROR: " + val.message; },
                                  [](const return_value& val) -> std::string
                                  { return "return value: " + std::string {val.type().name()}; },
                                  [](const bound_function& func) -> std::string { return func.first->string(); },
                                  [](const ::array& arr) -> std::string
                                  {
                                      std::vector<std::string> result;
                                      std::transform(arr.cbegin(),
                                                     arr.cend(),
                                                     std::back_inserter(result),
                                                     [](const object_ptr& obj) -> std::string
                                                     { return std::to_string(obj->value); });
                                      return fmt::format("[{}]", fmt::join(result, ", "));
                                  },
                                  [](const auto&) -> std::string { return "unknown"; }},
                      value);
}
}  // namespace std

auto unwrap_return_value(const object& obj) -> object
{
    if (obj.is<return_value>()) {
        return std::any_cast<object>(obj.as<return_value>());
    }
    return obj;
}

auto object::type_name() const -> std::string
{
    return std::visit(
        overloaded {
            [](const nullvalue&) { return "nil"; },
            [](const bool) { return "bool"; },
            [](const integer_value) { return "integer"; },
            [](const string_value&) { return "string"; },
            [](const error&) { return "error"; },
            [](const return_value&) { return "return value"; },
            [](const bound_function&) { return "function"; },
            [](const array&) { return "array"; },
            [](const auto&) { return "unknown"; },
        },
        value);
}
inline object::object(value_type val)
    : value {std::move(val)}
{
}
