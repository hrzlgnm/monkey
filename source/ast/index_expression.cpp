#include <string>

#include "index_expression.hpp"

#include <fmt/format.h>

#include "visitor.hpp"

auto index_expression::string() const -> std::string
{
    return fmt::format("({}[{}])", left->string(), index->string());
}

void index_expression::accept(visitor& visitor) const
{
    visitor.visit(*this);
}
