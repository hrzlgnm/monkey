#include <string>

#include "decimal_literal.hpp"

#include "util.hpp"
#include "visitor.hpp"

auto decimal_literal::string() const -> std::string
{
    return decimal_to_string(value);
}

void decimal_literal::accept(visitor& visitor) const
{
    visitor.visit(*this);
}
