#pragma once

#include "unary_expression.hpp"

struct binary_expression : unary_expression
{
    [[nodiscard]] auto string() const -> std::string override;
    [[nodiscard]] auto eval(environment_ptr env) const -> object_ptr override;
    auto compile(compiler& comp) const -> void override;

    expression_ptr left {};
};
