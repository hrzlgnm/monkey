#include <complex>

#include "call_expression.hpp"

#include "util.hpp"

auto call_expression::string() const -> std::string
{
    return fmt::format("{}({})", function->string(), join(arguments, ", "));
}
