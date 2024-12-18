#include <string>

#include "decimal_literal.hpp"

auto decimal_literal::string() const -> std::string
{
    return std::to_string(value);
}
