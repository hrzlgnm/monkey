#include <string>

#include "unary_expression.hpp"

#include <fmt/format.h>

auto unary_expression::string() const -> std::string
{
    return fmt::format("({}{})", op, right->string());
}
