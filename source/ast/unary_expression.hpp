#pragma once

#include <lexer/token_type.hpp>

#include "expression.hpp"

struct unary_expression : expression
{
    auto string() const -> std::string override;
    auto eval(environment_ptr env) const -> object override;
    auto compile(compiler& comp) const -> void override;

    token_type op {};
    expression_ptr right {};
};
