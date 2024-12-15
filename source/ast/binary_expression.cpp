#include "binary_expression.hpp"

#include <fmt/format.h>

auto binary_expression::string() const -> std::string
{
    return fmt::format("({} {} {})", left->string(), op, right->string());
}
