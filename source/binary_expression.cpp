#include "binary_expression.hpp"

#include <fmt/core.h>

#include "object.hpp"

auto binary_expression::string() const -> std::string
{
    return fmt::format("({} {} {})", left->string(), op, right->string());
}

auto eval_integer_binary_expression(token_type oper, const object& left, const object& right) -> object
{
    using enum token_type;
    auto left_int = left.as<integer_value>();
    auto right_int = right.as<integer_value>();
    switch (oper) {
        case plus:
            return {left_int + right_int};
        case minus:
            return {left_int - right_int};
        case asterisk:
            return {left_int * right_int};
        case slash:
            return {left_int / right_int};
        case less_than:
            return {left_int < right_int};
        case greater_than:
            return {left_int > right_int};
        case equals:
            return {left_int == right_int};
        case not_equals:
            return {left_int != right_int};
        default:
            return {};
    }
}

auto binary_expression::eval(environment_ptr env) const -> object
{
    using enum token_type;
    auto evaluated_left = left->eval(env);
    if (evaluated_left.is<error>()) {
        return evaluated_left;
    }
    auto evaluated_right = right->eval(env);
    if (evaluated_right.is<error>()) {
        return evaluated_right;
    }
    if (evaluated_left.type_name() != evaluated_right.type_name()) {
        return {error {.message = fmt::format(
                           "type mismatch: {} {} {}", evaluated_left.type_name(), op, evaluated_right.type_name())}};
    }
    if (evaluated_left.is<integer_value>() && evaluated_right.is<integer_value>()) {
        return eval_integer_binary_expression(op, evaluated_left, evaluated_right);
    }
    return {error {.message = fmt::format(
                       "unknown operator: {} {} {}", evaluated_left.type_name(), op, evaluated_right.type_name())}};
}
