#pragma once

#include "expression.hpp"

struct index_expression : expression
{
    auto string() const -> std::string override;
    auto eval(environment_ptr env) const -> object override;
    // auto compile(compiler& comp) const -> void override;

    expression_ptr left;
    expression_ptr index;
};
