#include "expression.hpp"

expression::expression(token tokn)
    : tkn {tokn}
{
}

auto expression::token_literal() const -> std::string_view
{
    return tkn.literal;
}
