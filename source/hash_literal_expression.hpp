#pragma once

#include <utility>
#include <vector>

#include "expression.hpp"

struct hash_literal_expression : expression
{
    using expression::expression;
    auto string() const -> std::string override;
    auto eval(environment_ptr env) const -> object override;
    std::vector<std::pair<expression_ptr, expression_ptr>> pairs;
};
