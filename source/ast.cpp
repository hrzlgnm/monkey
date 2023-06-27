#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

#include "ast.hpp"

#include "object.hpp"
#include "token.hpp"
#include "token_type.hpp"

auto eval_not_implemented_yet(const std::string& cls) -> object
{
    throw std::runtime_error(cls + "::eval() not implemented yet");
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

auto program::eval() const -> object
{
    object result;
    for (const auto& statement : statements) {
        result = statement->eval();
    }
    return result;
}

identifier::identifier(token tokn, std::string_view val)
    : expression {tokn}
    , value {val}
{
}
auto identifier::eval() const -> object
{
    return eval_not_implemented_yet(typeid(*this).name());
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

auto let_statement::eval() const -> object
{
    return eval_not_implemented_yet(typeid(*this).name());
}

auto return_statement::string() const -> std::string
{
    std::stringstream strm;
    strm << token_literal() << " ";
    if (return_value) {
        strm << return_value->string();
    }
    strm << ';';
    return strm.str();
}

auto return_statement::eval() const -> object
{
    return eval_not_implemented_yet(typeid(*this).name());
}

auto expression_statement::string() const -> std::string
{
    if (expr) {
        return expr->string();
    }
    return {};
}

auto expression_statement::eval() const -> object
{
    if (expr) {
        return expr->eval();
    }
    return {};
}

boolean::boolean(token tokn, bool val)
    : expression {tokn}
    , value {val}
{
}

auto boolean::eval() const -> object
{
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

auto integer_literal::eval() const -> object
{
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

auto unary_expression::eval() const -> object
{
    using enum token_type;
    auto evaluated_value = right->eval();
    switch (op) {
        case minus:
            return {-evaluated_value.as<integer_value>()};
        case exclamation:
            if (!evaluated_value.is<bool>()) {
                return {false};
            }
            return {!evaluated_value.as<bool>()};
        default:
            std::ostringstream ostrm;
            ostrm << op;
            return eval_not_implemented_yet(ostrm.str() + " " + typeid(*this).name());
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

auto binary_expression::eval() const -> object
{
    using enum token_type;
    auto evaluated_left = left->eval();
    auto evaluated_right = right->eval();

    switch (op) {
        case plus:
            return {evaluated_left.as<integer_value>() + evaluated_right.as<integer_value>()};
        case minus:
            return {evaluated_left.as<integer_value>() - evaluated_right.as<integer_value>()};
        case asterisk:
            return {evaluated_left.as<integer_value>() * evaluated_right.as<integer_value>()};
        case slash:
            return {evaluated_left.as<integer_value>() / evaluated_right.as<integer_value>()};
        case less_than:
            return {evaluated_left.as<integer_value>() < evaluated_right.as<integer_value>()};
        case greater_than:
            return {evaluated_left.as<integer_value>() > evaluated_right.as<integer_value>()};
        case equals:
            return {evaluated_left == evaluated_right};
        case not_equals:
            return {evaluated_left != evaluated_right};
        default:
            std::ostringstream ostrm;
            ostrm << evaluated_left.type_name() << " " << op << " " << evaluated_right.type_name();
            return eval_not_implemented_yet(ostrm.str() + " " + typeid(*this).name());
    }
}

auto block_statement::string() const -> std::string
{
    std::ostringstream strm;
    for (const auto& stmt : statements) {
        strm << stmt->string();
    }
    return strm.str();
}

auto block_statement::eval() const -> object
{
    object result;
    for (const auto& statement : statements) {
        result = statement->eval();
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

auto if_expression::eval() const -> object
{
    auto evaluated_condition = condition->eval();
    if (is_truthy(evaluated_condition)) {
        return consequence->eval();
    }
    if (alternative) {
        return alternative->eval();
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

auto function_literal::eval() const -> object
{
    return eval_not_implemented_yet(typeid(*this).name());
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

auto call_expression::eval() const -> object
{
    return eval_not_implemented_yet(typeid(*this).name());
}
