#include <string>

#include "array_literal.hpp"

#include <fmt/format.h>

#include "util.hpp"

auto array_literal::string() const -> std::string
{
    return fmt::format("[{}]", join(elements, ", "));
}
