#pragma once

#include <lexer/token_type.hpp>

#include "expression.hpp"

struct unary_expression : expression
{
    [[nodiscard]] auto string() const -> std::string override;
    void accept(struct visitor& visitor) const override;
    auto compile(compiler& comp) const -> void override;

    token_type op {};
    expression* right {};
};
