#include <algorithm>
#include <string>

#include "object.hpp"

#include <ast/callable_expression.hpp>
#include <fmt/format.h>

auto to_string(const hash_key_type& hash_key)
{
    return std::visit(overloaded {[](const integer_type val) -> std::string { return std::to_string(val); },
                                  [](const string_type& val) -> std::string { return "\"" + val + "\""; },
                                  [](const bool val) -> std::string { return val ? "true" : "false"; },
                                  [](const auto&) -> std::string { return "unknown"; }},
                      hash_key);
}

auto operator==(const error& lhs, const error& rhs) -> bool
{
    return lhs.message == rhs.message;
}

namespace std
{
auto to_string(const value_type& value) -> std::string
{
    return std::visit(
        overloaded {[](const null_type&) -> std::string { return "nil"; },
                    [](const integer_type val) -> std::string { return to_string(val); },
                    [](const string_type& val) -> std::string { return "\"" + val + "\""; },
                    [](const bool val) -> std::string { return val ? "true" : "false"; },
                    [](const error& val) -> std::string { return "ERROR: " + val.message; },
                    [](const ::array& arr) -> std::string
                    {
                        std::vector<std::string> result;
                        std::transform(arr.cbegin(),
                                       arr.cend(),
                                       std::back_inserter(result),
                                       [](const object& obj) -> std::string { return std::to_string(obj.value); });
                        return fmt::format("[{}]", fmt::join(result, ", "));
                    },
                    [](const ::hash& hsh) -> std::string
                    {
                        std::vector<std::string> pairs;
                        std::transform(hsh.cbegin(),
                                       hsh.cend(),
                                       std::back_inserter(pairs),
                                       [](const auto& pair) {
                                           return fmt::format("{}: {}",
                                                              ::to_string(pair.first),
                                                              to_string(std::any_cast<object>(pair.second).value));
                                       });
                        return fmt::format("{{{}}}", fmt::join(pairs, ", "));
                    },
                    [](const bound_function& func) -> std::string { return func.first->string(); },
                    [](const compiled_function& compf) -> std::string
                    { return fmt::format("compiled[{},{}]", compf.instrs.size(), compf.num_locals); },
                    [](const closure& clsr) -> std::string
                    { return fmt::format("closure[{},{}]", clsr.fn.instrs.size(), clsr.fn.num_locals); }},

        value);
}
}  // namespace std

auto object::type_name() const -> std::string
{
    if (is_return_value) {
        return "return value";
    }
    return std::visit(
        overloaded {
            [](const null_type&) { return "nil"; },
            [](const bool) { return "bool"; },
            [](const integer_type) { return "integer"; },
            [](const string_type&) { return "string"; },
            [](const error&) { return "error"; },
            [](const bound_function&) { return "function"; },
            [](const array&) { return "array"; },
            [](const hash&) { return "hash"; },
            [](const compiled_function&) { return "compiled_function"; },
            [](const closure&) { return "closure"; },
        },
        value);
}

auto unwrap(const std::any& obj) -> object
{
    return std::any_cast<object>(obj);
}
