#include <string>
#include <utility>
#include <vector>

#include "callable_expression.hpp"

#include <fmt/format.h>
#include <fmt/ranges.h>

callable_expression::callable_expression(std::vector<std::string> params)
    : parameters {std::move(params)}
{
}

auto callable_expression::string() const -> std::string
{
    return fmt::format("fn({}){}", fmt::join(parameters, ", "), "{...}");
}
