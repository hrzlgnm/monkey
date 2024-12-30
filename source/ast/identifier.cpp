#include <string>

#include "identifier.hpp"

#include "visitor.hpp"

auto identifier::string() const -> std::string
{
    return value;
}

void identifier::accept(visitor& visitor) const
{
    visitor.visit(*this);
}
