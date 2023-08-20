#include "function_expression.hpp"

#include <fmt/format.h>

function_expression::function_expression(std::vector<std::string>&& parameters, statement_ptr&& body)
    : callable_expression(std::move(parameters))
    , body(std::move(body))
{
}

auto function_expression::string() const -> std::string
{
    return fmt::format("fn({}) {{ {}; }}", fmt::join(parameters, ", "), body->string());
}
