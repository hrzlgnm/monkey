#include "unary_expression.hpp"

#include <fmt/core.h>

auto unary_expression::string() const -> std::string
{
    return fmt::format("({}{})", op, right->string());
}
