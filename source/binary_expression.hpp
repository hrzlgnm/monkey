#pragma once

#include "unary_expression.hpp"

struct binary_expression : unary_expression
{
    auto string() const -> std::string override;
    auto eval(environment_ptr env) const -> object override;

    expression_ptr left {};
};
