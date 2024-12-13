#pragma once

#include <vector>

#include "expression.hpp"

struct call_expression : expression
{
    [[nodiscard]] auto string() const -> std::string override;
    [[nodiscard]] auto eval(environment* env) const -> const object* override;
    auto compile(compiler& comp) const -> void override;

    expression* function {};
    std::vector<const expression*> arguments;
};
