#include "unary_expression.hpp"

#include <fmt/core.h>

#include "environment.hpp"
#include "object.hpp"

auto unary_expression::string() const -> std::string
{
    return fmt::format("({}{})", op, right->string());
}

auto unary_expression::eval(environment_ptr env) const -> object
{
    using enum token_type;
    auto evaluated_value = right->eval(env);
    if (evaluated_value.is<error>()) {
        return evaluated_value;
    }
    switch (op) {
        case minus:
            if (!evaluated_value.is<integer_value>()) {
                return make_error("unknown operator: -{}", evaluated_value.type_name());
            }
            return {-evaluated_value.as<integer_value>()};
        case exclamation:
            if (!evaluated_value.is<bool>()) {
                return {false};
            }
            return {!evaluated_value.as<bool>()};
        default:
            return make_error("unknown operator: {}{}", op, evaluated_value.type_name());
    }
}
