#include <string>

#include "boolean_literal.hpp"

boolean_literal::boolean_literal(bool val)
    : value {val}
{
}

auto boolean_literal::string() const -> std::string
{
    return std::string {value ? "true" : "false"};
}
