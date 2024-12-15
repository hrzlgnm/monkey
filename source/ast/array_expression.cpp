#include <string>

#include "array_expression.hpp"

#include <fmt/format.h>

#include "util.hpp"

auto array_expression::string() const -> std::string
{
    return fmt::format("[{}]", join(elements, ", "));
}
