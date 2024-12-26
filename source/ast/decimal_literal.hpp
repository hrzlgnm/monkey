#pragma once

#include "expression.hpp"

struct decimal_literal final : expression
{
    [[nodiscard]] auto string() const -> std::string override;
    void accept(struct visitor& visitor) const final;

    double value {};
};
