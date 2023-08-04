#include "boolean.hpp"

#include "object.hpp"
#include "util.hpp"

boolean::boolean(token tokn, bool val)
    : expression {tokn}
    , value {val}
{
}

auto boolean::eval(environment_ptr /*env*/) const -> object
{
    return object {value};
}

auto boolean::string() const -> std::string
{
    return std::string {tkn.literal};
}
