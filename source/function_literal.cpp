#include "function_literal.hpp"

#include <fmt/core.h>

#include "environment.hpp"
#include "util.hpp"

auto function_literal::string() const -> std::string
{
    return fmt::format("{}({}) {}", token_literal(), join(parameters, ", "), body->string());
}

auto function_literal::eval(environment_ptr env) const -> object
{
    auto function_object = std::make_shared<fun>();
    function_object->parameters = parameters;
    function_object->body = body;
    function_object->env = env;
    return {function_object};
}
