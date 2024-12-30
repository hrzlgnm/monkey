#include <string>

#include "string_literal.hpp"

#include <fmt/format.h>

#include "visitor.hpp"

auto string_literal::string() const -> std::string
{
    return fmt::format(R"("{}")", value);
}

void string_literal::accept(visitor& visitor) const
{
    visitor.visit(*this);
}
