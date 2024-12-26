#pragma once

#include "unary_expression.hpp"

struct binary_expression : unary_expression
{
    [[nodiscard]] auto string() const -> std::string override;
    [[nodiscard]] auto eval(environment* env) const -> const object* override;
    auto compile(compiler& comp) const -> void override;
    auto check(symbol_table* symbols) const -> void override;

    expression* left {};
};
