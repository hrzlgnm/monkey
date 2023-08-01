#pragma once
#include <vector>

#include "environment_fwd.hpp"
#include "expression.hpp"

struct array_expression : expression
{
    using expression::expression;
    auto string() const -> std::string override;
    auto eval(environment_ptr env) const -> object override;

    std::vector<expression_ptr> elements;
};
