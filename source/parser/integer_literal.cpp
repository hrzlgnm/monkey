#include "integer_literal.hpp"

#include "compiler/code.hpp"
#include "compiler/compiler.hpp"
#include "object.hpp"
#include "util.hpp"

auto integer_literal::string() const -> std::string
{
    return std::to_string(value);
}

auto integer_literal::eval(environment_ptr /*env*/) const -> object
{
    return {value};
}
