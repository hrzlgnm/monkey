#pragma once
#include <vector>

#include "expression.hpp"

struct array_expression : expression
{
    [[nodiscard]] auto string() const -> std::string override;
    [[nodiscard]] auto eval(environment* env) const -> const object* override;
    auto compile(compiler& comp) const -> void override;

    std::vector<const expression*> elements;
};
