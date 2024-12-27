#include <string>

#include "call_expression.hpp"

#include <fmt/format.h>

#include "util.hpp"
#include "visitor.hpp"

auto call_expression::string() const -> std::string
{
    return fmt::format("{}({})", callee->string(), join(arguments, ", "));
}

void call_expression::accept(visitor& visitor) const
{
    visitor.visit(*this);
}
