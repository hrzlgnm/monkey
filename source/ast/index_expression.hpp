#pragma once

#include "expression.hpp"

struct index_expression : expression
{
    [[nodiscard]] auto string() const -> std::string override;
    [[nodiscard]] auto eval(environment* env) const -> const object* override;
    auto compile(compiler& comp) const -> void override;

    expression* left {};
    expression* index {};
};
