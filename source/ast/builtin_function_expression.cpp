#include "builtin_function_expression.hpp"

#include "eval/object.hpp"

builtin_function_expression::builtin_function_expression(
    std::string&& name,
    std::vector<std::string>&& params,
    std::function<object_ptr(array_object::array&& arguments)>&& bod)
    : callable_expression {std::move(params)}
    , name {std::move(name)}
    , body {std::move(bod)}
{
}

auto builtin_function_expression::string() const -> std::string
{
    return fmt::format("{}(){}", name, "{...}");
}
