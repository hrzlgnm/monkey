#pragma once
#include <vector>

#include "expression.hpp"

struct array_expression : expression
{
    [[nodiscard]] auto string() const -> std::string override;
    [[nodiscard]] auto eval(environment_ptr env) const -> object_ptr override;
    auto compile(compiler& comp) const -> void override;

    std::vector<expression_ptr> elements;
};
