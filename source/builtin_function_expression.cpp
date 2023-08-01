#include <algorithm>
#include <iterator>

#include "builtin_function_expression.hpp"

#include "environment.hpp"
#include "object.hpp"
#include "value_type.hpp"

const std::vector<builtin_function_expression> builtin_function_expression::builtins {
    {"len",
     {"val"},
     [](const std::vector<object>& arguments) -> object
     {
         if (arguments.size() != 1) {
             return make_error("wrong number of arguments to len(): expected=1, got={}", arguments.size());
         }
         return object {std::visit(
             overloaded {
                 [](const string_value& str) -> object { return {static_cast<integer_value>(str.length())}; },
                 [](const auto& other) -> object
                 { return make_error("argument of type {} to len() is not supported", object {other}.type_name()); },
             },
             arguments[0].value)};
     }},
};

builtin_function_expression::builtin_function_expression(
    std::string&& name,
    std::vector<std::string>&& parameters,
    std::function<object(const std::vector<object>& arguments)>&& body)
    : callable_expression {std::move(parameters)}
    , name {std::move(name)}
    , body {std::move(body)}
{
}
auto builtin_function_expression::call(environment_ptr /*closure_env*/,
                                       environment_ptr caller_env,
                                       const std::vector<expression_ptr>& arguments) const -> object
{
    std::vector<object> args;
    std::transform(arguments.cbegin(),
                   arguments.cend(),
                   std::back_inserter(args),
                   [&caller_env](const expression_ptr& expr) { return expr->eval(caller_env); });
    return body(std::move(args));
};

auto builtin_function_expression::string() const -> std::string
{
    return fmt::format("{}(){}", name, "{...}");
};
