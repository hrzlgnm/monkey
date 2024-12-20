#include <string>

#include "decimal_literal.hpp"

#include "util.hpp"

auto decimal_literal::string() const -> std::string
{
    return decimal_to_string(value);
}
