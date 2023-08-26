#include "character_literal.hpp"

character_literal::character_literal(char val)
    : value(val)
{
}

auto character_literal::string() const -> std::string
{
    return {1, value};
}
