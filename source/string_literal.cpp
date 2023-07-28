#include "string_literal.hpp"

#include "object.hpp"
#include "util.hpp"

auto string_literal::string() const -> std::string
{
    return std::string {tkn.literal};
}

auto string_literal::eval(environment_ptr env) const -> object
{
    unused(env);
    return object {value};
}
