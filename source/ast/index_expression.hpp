#pragma once

#include "expression.hpp"

struct index_expression final : expression
{
    [[nodiscard]] auto string() const -> std::string override;
    void accept(struct visitor& visitor) const final;
    auto compile(compiler& comp) const -> void override;

    expression* left {};
    expression* index {};
};
