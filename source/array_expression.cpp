#include "array_expression.hpp"

#include "object.hpp"
#include "util.hpp"

auto array_expression::string() const -> std::string
{
    return fmt::format("[{}]", join(elements, ", "));
}

auto array_expression::eval(environment_ptr env) const -> object
{
    unused(env);
    return {};
}
