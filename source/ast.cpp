#include <algorithm>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

#include "ast.hpp"

#include <fmt/core.h>

#include "environment.hpp"
#include "node.hpp"
#include "object.hpp"
#include "program.hpp"
#include "token.hpp"
#include "token_type.hpp"
#include "value_type.hpp"

template<typename T>
void unused(T /*unused*/)
{
}

auto eval_not_implemented_yet(const std::string& cls) -> object
{
    return {error {.message = fmt::format("cannot evaluate: {}", cls)}};
}

statement::statement(token tokn)
    : tkn {tokn}
{
}

auto statement::token_literal() const -> std::string_view
{
    return tkn.literal;
}

expression::expression(token tokn)
    : tkn {tokn}
{
}

auto expression::token_literal() const -> std::string_view
{
    return tkn.literal;
}

auto program::token_literal() const -> std::string_view
{
    if (statements.empty()) {
        return "";
    }
    return statements[0]->token_literal();
}

auto program::string() const -> std::string
{
    std::stringstream strm;
    for (const auto& statement : statements) {
        strm << statement->string();
    }
    return strm.str();
}

auto program::eval(environment_ptr env) const -> object
{
    object result;
    for (const auto& statement : statements) {
        result = statement->eval(env);
        if (result.is<return_value>()) {
            return std::any_cast<object>(result.as<return_value>());
        }
        if (result.is<error>()) {
            return result;
        }
    }
    return result;
}

identifier::identifier(token tokn, std::string_view val)
    : expression {tokn}
    , value {val}
{
}
auto identifier::eval(environment_ptr env) const -> object
{
    auto val = env->get(value);
    if (!val) {
        return {error {.message = fmt::format("identifier not found: {}", value)}};
    }
    return val.value();
}

auto identifier::string() const -> std::string
{
    return {value.data(), value.size()};
}

auto let_statement::string() const -> std::string
{
    std::stringstream strm;
    strm << token_literal() << " " << name->string() << " = ";
    if (value) {
        strm << value->string();
    }
    strm << ';';
    return strm.str();
}

auto let_statement::eval(environment_ptr env) const -> object
{
    auto val = value->eval(env);
    if (val.is<error>()) {
        return val;
    }
    return env->set(std::string(name->value), val);
}

auto return_statement::string() const -> std::string
{
    std::stringstream strm;
    strm << token_literal() << " ";
    if (value) {
        strm << value->string();
    }
    strm << ';';
    return strm.str();
}

auto return_statement::eval(environment_ptr env) const -> object
{
    if (value) {
        auto evaluated = value->eval(env);
        if (evaluated.is<error>()) {
            return evaluated;
        }
        return {.value = std::make_any<object>(evaluated)};
    }
    return {};
}

auto expression_statement::string() const -> std::string
{
    if (expr) {
        return expr->string();
    }
    return {};
}

auto expression_statement::eval(environment_ptr env) const -> object
{
    if (expr) {
        return expr->eval(env);
    }
    return {};
}

boolean::boolean(token tokn, bool val)
    : expression {tokn}
    , value {val}
{
}

auto boolean::eval(environment_ptr env) const -> object
{
    unused(env);
    return object {value};
}

auto boolean::string() const -> std::string
{
    return std::string {tkn.literal};
}

auto integer_literal::string() const -> std::string
{
    return std::string {tkn.literal};
}

auto integer_literal::eval(environment_ptr env) const -> object
{
    unused(env);
    return object {value};
}

auto unary_expression::string() const -> std::string
{
    std::ostringstream strm;
    strm << "(";
    strm << op;
    strm << right->string();
    strm << ")";
    return strm.str();
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
                return {error {.message = fmt::format("unknown operator: -{}", evaluated_value.type_name())}};
            }
            return {-evaluated_value.as<integer_value>()};
        case exclamation:
            if (!evaluated_value.is<bool>()) {
                return {false};
            }
            return {!evaluated_value.as<bool>()};
        default:
            return {
                error {.message = fmt::format("unknown operator: {}{}", to_string(op), evaluated_value.type_name())}};
    }
}

auto binary_expression::string() const -> std::string
{
    std::ostringstream strm;
    strm << "(";
    strm << left->string();
    strm << ' ';
    strm << op;
    strm << ' ';
    strm << right->string();
    strm << ")";
    return strm.str();
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
        return {error {
            .message = fmt::format(
                "type mismatch: {} {} {}", evaluated_left.type_name(), to_string(op), evaluated_right.type_name())}};
    }
    if (evaluated_left.is<integer_value>() && evaluated_right.is<integer_value>()) {
        return eval_integer_binary_expression(op, evaluated_left, evaluated_right);
    }
    return {error {
        .message = fmt::format(
            "unknown operator: {} {} {}", evaluated_left.type_name(), to_string(op), evaluated_right.type_name())}};
}

auto block_statement::string() const -> std::string
{
    std::ostringstream strm;
    for (const auto& stmt : statements) {
        strm << stmt->string();
    }
    return strm.str();
}

auto block_statement::eval(environment_ptr env) const -> object
{
    object result;
    for (const auto& stmt : statements) {
        result = stmt->eval(env);
        if (result.is<return_value>() || result.is<error>()) {
            return result;
        }
    }
    return result;
}

auto if_expression::string() const -> std::string
{
    std::ostringstream strm;
    strm << "if" << condition->string() << " " << consequence->string();
    if (alternative) {
        strm << "else " << alternative->string();
    }
    return strm.str();
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

auto function_literal::string() const -> std::string
{
    std::ostringstream strm;
    strm << tkn.literal << "(";
    bool first = true;
    for (const auto& param : parameters) {
        if (!first) {
            strm << ", ";
        }
        strm << param->string();
        first = false;
    }
    strm << ") ";
    strm << body->string();
    return strm.str();
}

auto function_literal::eval(environment_ptr env) const -> object
{
    auto function_object = std::make_shared<fun>();
    function_object->parameters = parameters;
    function_object->body = body;
    function_object->env = env;
    return {function_object};
}

auto call_expression::string() const -> std::string
{
    std::ostringstream strm;

    strm << function->string() << "(";
    bool first = true;
    for (const auto& argument : arguments) {
        if (!first) {
            strm << ", ";
        }
        strm << argument->string();
        first = false;
    }
    strm << ")";

    return strm.str();
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
