#pragma once

#include "expression.hpp"

struct unary_expression : expression
{
    using expression::expression;
    auto string() const -> std::string override;
    auto eval(environment_ptr env) const -> object override;

    token_type op {};
    expression_ptr right {};
};
