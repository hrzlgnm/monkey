#include <algorithm>
#include <sstream>
#include <string>

#include "ast.hpp"

#include "token.hpp"
#include "token_type.hpp"

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

identifier::identifier(token tokn, std::string_view val)
    : expression {tokn}
    , value {val}
{
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

auto expression_statement::string() const -> std::string
{
    if (expr) {
        return expr->string();
    }
    return {};
}

boolean::boolean(token tokn, bool val)
    : expression {tokn}
    , value {val}
{
}

auto boolean::string() const -> std::string
{
    return std::string {tkn.literal};
}

auto integer_literal::string() const -> std::string
{
    return std::string {tkn.literal};
}

auto prefix_expression::string() const -> std::string
{
    std::ostringstream strm;
    strm << "(";
    strm << op;
    strm << right->string();
    strm << ")";
    return strm.str();
}

auto infix_expression::string() const -> std::string
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

auto block_statement::string() const -> std::string
{
    std::ostringstream strm;
    for (const auto& stmt : statements) {
        strm << stmt->string();
    }
    return strm.str();
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
