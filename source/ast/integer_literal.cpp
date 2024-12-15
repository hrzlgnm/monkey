#include <string>

#include "integer_literal.hpp"

auto integer_literal::string() const -> std::string
{
    return std::to_string(value);
}
