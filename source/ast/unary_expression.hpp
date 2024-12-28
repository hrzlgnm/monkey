#pragma once

#include <lexer/token_type.hpp>

#include "expression.hpp"

struct unary_expression final : expression
{
    [[nodiscard]] auto string() const -> std::string final;
    void accept(struct visitor& visitor) const final;

    token_type op {};
    expression* right {};
};
