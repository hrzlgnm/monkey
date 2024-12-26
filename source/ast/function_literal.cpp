#include <string>
#include <utility>
#include <vector>

#include "function_literal.hpp"

#include <fmt/format.h>
#include <fmt/ranges.h>

#include "callable_expression.hpp"
#include "statements.hpp"

function_literal::function_literal(std::vector<std::string>&& params, const statement* bod)
    : callable_expression(std::move(params))
    , body {bod}
{
}

auto function_literal::string() const -> std::string
{
    return fmt::format("fn({}) {{ {}; }}", fmt::join(parameters, ", "), body->string());
}
