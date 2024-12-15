#include <string>

#include "call_expression.hpp"

#include <fmt/format.h>

#include "util.hpp"

auto call_expression::string() const -> std::string
{
    return fmt::format("{}({})", function->string(), join(arguments, ", "));
}
