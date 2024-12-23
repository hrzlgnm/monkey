#pragma once

#include "expression.hpp"
#include "identifier.hpp"

struct assign_expression : expression
{
    [[nodiscard]] auto string() const -> std::string override;
    [[nodiscard]] auto eval(environment* env) const -> const object* override;
    auto compile(compiler& comp) const -> void override;

    const identifier* name {};
    const expression* value {};
};
