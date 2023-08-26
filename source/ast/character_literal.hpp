#pragma once

#include "expression.hpp"

struct character_literal : expression
{
    explicit character_literal(char val);
    [[nodiscard]] auto string() const -> std::string override;
    [[nodiscard]] auto eval(environment_ptr env) const -> object override;
    auto compile(compiler& comp) const -> void override;

    char value;
};
