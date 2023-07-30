#include "callable_expression.hpp"

#include <fmt/format.h>

#include "object.hpp"

callable_expression::callable_expression(std::vector<std::string>&& parameters)
    : parameters {std::move(parameters)}
{
}

auto callable_expression::eval(environment_ptr env) const -> object
{
    return {std::make_pair(this, env)};
}

auto callable_expression::string() const -> std::string
{
    return fmt::format("fn({}){}", fmt::join(parameters, ", "), "{...}");
}
