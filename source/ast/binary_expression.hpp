#pragma once

#include "unary_expression.hpp"

struct binary_expression final : unary_expression
{
    [[nodiscard]] auto string() const -> std::string override;
    void accept(struct visitor& visitor) const final;
    auto compile(compiler& comp) const -> void override;
    auto check(symbol_table* symbols) const -> void override;

    expression* left {};
};
