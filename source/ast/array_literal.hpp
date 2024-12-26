#pragma once
#include <vector>

#include "expression.hpp"

struct array_literal final : expression
{
    [[nodiscard]] auto string() const -> std::string override;
    void accept(struct visitor& visitor) const final;

    std::vector<const expression*> elements;
};
