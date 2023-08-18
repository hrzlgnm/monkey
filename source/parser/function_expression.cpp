#include <memory>

#include "function_expression.hpp"

#include "callable_expression.hpp"
#include "environment.hpp"
#include "object.hpp"
#include "util.hpp"

function_expression::function_expression(std::vector<std::string>&& parameters, statement_ptr&& body)
    : callable_expression(std::move(parameters))
    , body(std::move(body))
{
}
auto function_expression::string() const -> std::string
{
    return fmt::format("fn({}) {{ {}; }}", fmt::join(parameters, ", "), body->string());
}

auto function_expression::call(environment_ptr closure_env,
                               environment_ptr caller_env,
                               const std::vector<expression_ptr>& arguments) const -> object
{
    auto locals = std::make_shared<environment>(closure_env);
    for (auto arg_itr = arguments.begin(); const auto& parameter : parameters) {
        if (arg_itr != arguments.end()) {
            const auto& arg = *(arg_itr++);
            locals->set(parameter, arg->eval(caller_env));
        } else {
            locals->set(parameter, {});
        }
    }
    return body->eval(locals);
}
