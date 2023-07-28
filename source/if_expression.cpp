#include "if_expression.hpp"

#include <fmt/core.h>

#include "object.hpp"

auto if_expression::string() const -> std::string
{
    return fmt::format("if {} {} {}",
                       condition->string(),
                       consequence->string(),
                       alternative ? fmt::format("else {}", alternative->string()) : std::string());
}

inline auto is_truthy(const object& obj) -> bool
{
    return !obj.is<nullvalue>() && (!obj.is<bool>() || obj.as<bool>());
}

auto if_expression::eval(environment_ptr env) const -> object
{
    auto evaluated_condition = condition->eval(env);
    if (evaluated_condition.is<error>()) {
        return evaluated_condition;
    }
    if (is_truthy(evaluated_condition)) {
        return consequence->eval(env);
    }
    if (alternative) {
        return alternative->eval(env);
    }
    return {};
}
