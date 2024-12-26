#include <string>

#include "assign_expression.hpp"

#include <fmt/format.h>

#include "visitor.hpp"

auto assign_expression::string() const -> std::string
{
    return fmt::format("{} = {};", name->string(), value->string());
}

void assign_expression::accept(visitor& visitor) const
{
    visitor.visit(*this);
}
