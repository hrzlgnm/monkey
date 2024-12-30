#include <string>

#include "function_literal.hpp"

#include <fmt/format.h>

#include "statements.hpp"
#include "util.hpp"
#include "visitor.hpp"

auto function_literal::string() const -> std::string
{
    return fmt::format("fn({}) {{ {}; }}", join(parameters, ", "), body->string());
}

void function_literal::accept(visitor& visitor) const
{
    visitor.visit(*this);
}
