#include <complex>

#include "call_expression.hpp"

#include "callable_expression.hpp"
#include "environment.hpp"
#include "object.hpp"
#include "util.hpp"

auto call_expression::string() const -> std::string
{
    return fmt::format("{}({})", function->string(), join(arguments, ", "));
}

auto call_expression::eval(environment_ptr env) const -> object
{
    auto evaluated = function->eval(env);
    if (evaluated.is<error>()) {
        return evaluated;
    }
    auto [fn, closure_env] = evaluated.as<bound_function>();
    return fn->call(closure_env, env, arguments);
}
