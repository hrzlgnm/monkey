#include <string>

#include "binary_expression.hpp"

#include <fmt/format.h>

#include "visitor.hpp"

auto binary_expression::string() const -> std::string
{
    return fmt::format("({} {} {})", left->string(), op, right->string());
}

void binary_expression::accept(visitor& visitor) const
{
    visitor.visit(*this);
}
