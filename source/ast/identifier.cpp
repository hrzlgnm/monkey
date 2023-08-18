#include "identifier.hpp"

identifier::identifier(std::string val)
    : value {std::move(val)}
{
}
auto identifier::string() const -> std::string
{
    return value;
}
