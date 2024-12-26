#include <string>
#include <utility>

#include "identifier.hpp"

#include "visitor.hpp"

identifier::identifier(std::string val)
    : value {std::move(val)}
{
}

auto identifier::string() const -> std::string
{
    return value;
}

void identifier::accept(visitor& visitor) const
{
    visitor.visit(*this);
}
