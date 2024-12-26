#include <string>

#include "integer_literal.hpp"

#include "visitor.hpp"

auto integer_literal::string() const -> std::string
{
    return std::to_string(value);
}

void integer_literal::accept(visitor& visitor) const
{
    visitor.visit(*this);
}
