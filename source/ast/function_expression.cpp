#include <string>
#include <utility>
#include <vector>

#include "function_expression.hpp"

#include <fmt/format.h>
#include <fmt/ranges.h>

#include "callable_expression.hpp"
#include "statements.hpp"

function_expression::function_expression(std::vector<std::string>&& params, const statement* bod)
    : callable_expression(std::move(params))
    , body {bod}
{
}

auto function_expression::string() const -> std::string
{
    return fmt::format("fn({}) {{ {}; }}", fmt::join(parameters, ", "), body->string());
}
