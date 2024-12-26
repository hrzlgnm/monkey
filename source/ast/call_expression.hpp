#pragma once

#include "expression.hpp"

struct call_expression final : expression
{
    [[nodiscard]] auto string() const -> std::string override;
    void accept(struct visitor& visitor) const final;
    auto compile(compiler& comp) const -> void override;

    expression* function {};
    expressions arguments;
};
