#include <string>

#include "string_literal.hpp"

#include "visitor.hpp"

auto string_literal::string() const -> std::string
{
    return value;
}

void string_literal::accept(visitor& visitor) const
{
    visitor.visit(*this);
}
