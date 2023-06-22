#include <algorithm>
#include <sstream>

#include "ast.hpp"

#include "token.hpp"
#include "token_type.hpp"

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
    : tkn {tokn}
    , value {val}
{
}

auto identifier::token_literal() const -> std::string_view
{
    return tkn.literal;
}
auto identifier::string() const -> std::string
{
    return {value.data(), value.size()};
}

auto let_statement::token_literal() const -> std::string_view
{
    return tkn.literal;
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

auto return_statement::token_literal() const -> std::string_view
{
    return tkn.literal;
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

auto expression_statement::token_literal() const -> std::string_view
{
    return tkn.literal;
}
auto expression_statement::string() const -> std::string
{
    if (expr) {
        return expr->string();
    }
    return {};
}

auto integer_literal::token_literal() const -> std::string_view
{
    return tkn.literal;
}

auto integer_literal::string() const -> std::string
{
    return std::string {tkn.literal};
}

auto prefix_expression::token_literal() const -> std::string_view
{
    return tkn.literal;
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
