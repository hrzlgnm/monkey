#pragma once

#include "expression.hpp"

struct binary_expression : expression
{
    using expression::expression;
    auto string() const -> std::string override;
    auto eval(environment_ptr env) const -> object override;

    expression_ptr left {};
    token_type op {};
    expression_ptr right {};
};
