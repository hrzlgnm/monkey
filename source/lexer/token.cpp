#include <ostream>

#include "token.hpp"

auto token::operator==(const token& other) const -> bool
{
    return type == other.type && literal == other.literal;
}

auto operator<<(std::ostream& ostream, const token& token) -> std::ostream&
{
    return ostream << "token{" << token.type << ", `" << token.literal << "´}";
}
