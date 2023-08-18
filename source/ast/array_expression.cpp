#include "array_expression.hpp"

#include "util.hpp"

auto array_expression::string() const -> std::string
{
    return fmt::format("[{}]", join(elements, ", "));
}
