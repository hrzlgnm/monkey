#include "call_expression.hpp"

#include "environment.hpp"
#include "object.hpp"
#include "util.hpp"

auto call_expression::string() const -> std::string
{
    return fmt::format("{}({})", function->string(), join(arguments, ", "));
}

auto evaluate_expressions(const std::vector<expression_ptr>& expressions, const environment_ptr& env)
    -> std::vector<object>
{
    std::vector<object> result;
    for (const auto& expr : expressions) {
        const auto evaluated = expr->eval(env);
        if (evaluated.is<error>()) {
            return {evaluated};
        }
        result.emplace_back(evaluated);
    }
    return result;
}

auto extended_function_environment(const object& funci, const std::vector<object>& args) -> environment_ptr
{
    const auto& as_func = funci.as<func>();
    auto env = std::make_shared<enclosing_environment>(as_func->env);
    size_t idx = 0;
    for (const auto& parameter : as_func->parameters) {
        env->set(parameter->value, args[idx]);
        idx++;
    }
    return env;
}

auto unwrap_result(const object& obj) -> object
{
    if (obj.is<return_value>()) {
        return std::any_cast<object>(obj.as<return_value>());
    }
    return obj;
}

auto apply_function(const object& funct, const std::vector<object>& args) -> object
{
    if (!funct.is<func>()) {
        return {error {.message = fmt::format("not a function: {}", funct.type_name())}};
    }
    auto extended_env = extended_function_environment(funct, args);
    auto evaluated = funct.as<func>()->body->eval(extended_env);
    return unwrap_result(evaluated);
}

auto call_expression::eval(environment_ptr env) const -> object
{
    auto evaluated = function->eval(env);
    if (evaluated.is<error>()) {
        return evaluated;
    }
    auto args = evaluate_expressions(arguments, env);
    if (args.size() == 1 && args[0].is<error>()) {
        return args[0];
    }
    return apply_function(evaluated, args);
}
