#pragma once
#include <string>

#include <lexer/token_type.hpp>

#include "expression.hpp"

struct binary_expression final : expression
{
    using expression::expression;
    [[nodiscard]] auto string() const -> std::string final;
    void accept(struct visitor& visitor) const final;

    expression* left {};
    token_type op {};
    expression* right {};
};
