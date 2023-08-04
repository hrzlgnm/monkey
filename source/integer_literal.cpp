#include "integer_literal.hpp"

#include "object.hpp"
#include "util.hpp"

auto integer_literal::string() const -> std::string
{
    return std::string {tkn.literal};
}

auto integer_literal::eval(environment_ptr /*env*/) const -> object
{
    return object {value};
}
