#include <string>

#include "array_literal.hpp"

#include <fmt/format.h>

#include "util.hpp"
#include "visitor.hpp"

auto array_literal::string() const -> std::string
{
    return fmt::format("[{}]", join(elements, ", "));
}

void array_literal::accept(visitor& visitor) const
{
    visitor.visit(*this);
}
