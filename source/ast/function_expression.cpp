#include <string>

#include "function_expression.hpp"

#include <fmt/format.h>
#include <fmt/ranges.h>

function_expression::function_expression(std::vector<std::string>&& parameters, statement* body)
    : callable_expression(std::move(parameters))
    , body {body}
{
}

auto function_expression::string() const -> std::string
{
    return fmt::format("fn({}) {{ {}; }}", fmt::join(parameters, ", "), body->string());
}
