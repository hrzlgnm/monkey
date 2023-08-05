#pragma once

#include "expression.hpp"
#include "token_type.hpp"

struct unary_expression : expression
{
    auto string() const -> std::string override;
    auto eval(environment_ptr env) const -> object override;

    token_type op {};
    expression_ptr right {};
};
