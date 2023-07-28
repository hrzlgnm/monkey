#pragma once

#include <vector>

#include "expression.hpp"

struct call_expression : expression
{
    using expression::expression;

    auto string() const -> std::string override;
    auto eval(environment_ptr env) const -> object override;

    expression_ptr function {};
    std::vector<expression_ptr> arguments;
};
