#include "boolean.hpp"

boolean::boolean(bool val)
    : value {val}
{
}

auto boolean::string() const -> std::string
{
    return std::string {value ? "true" : "false"};
}
