#include <stdexcept>

#include "binary_expression.hpp"

#include <fmt/core.h>

#include "code.hpp"
#include "compiler.hpp"
#include "object.hpp"
#include "token.hpp"
#include "token_type.hpp"

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

auto eval_string_binary_expression(token_type oper, const object& left, const object& right) -> object
{
    using enum token_type;
    if (oper != plus) {
        return make_error("unknown operator: {} {} {}", left.type_name(), oper, right.type_name());
    }
    const auto& left_str = left.as<string_value>();
    const auto& right_str = right.as<string_value>();
    return {left_str + right_str};
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
        return make_error("type mismatch: {} {} {}", evaluated_left.type_name(), op, evaluated_right.type_name());
    }
    if (evaluated_left.is<integer_value>() && evaluated_right.is<integer_value>()) {
        return eval_integer_binary_expression(op, evaluated_left, evaluated_right);
    }
    if (evaluated_left.is<string_value>() && evaluated_right.is<string_value>()) {
        return eval_string_binary_expression(op, evaluated_left, evaluated_right);
    }
    return make_error("unknown operator: {} {} {}", evaluated_left.type_name(), op, evaluated_right.type_name());
}

auto binary_expression::compile(compiler& comp) const -> void
{
    if (op == token_type::less_than) {
        right->compile(comp);
        left->compile(comp);
        comp.emit(opcodes::greater_than);
        return;
    }
    left->compile(comp);
    right->compile(comp);
    switch (op) {
        case token_type::plus:
            comp.emit(opcodes::add);
            break;
        case token_type::minus:
            comp.emit(opcodes::sub);
            break;
        case token_type::asterisk:
            comp.emit(opcodes::mul);
            break;
        case token_type::slash:
            comp.emit(opcodes::div);
            break;
        case token_type::greater_than:
            comp.emit(opcodes::greater_than);
            break;
        case token_type::equals:
            comp.emit(opcodes::equal);
            break;
        case token_type::not_equals:
            comp.emit(opcodes::not_equal);
            break;
        default:
            throw std::runtime_error(fmt::format("unsupported operator {}", op));
    }
}
