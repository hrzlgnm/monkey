#pragma once

#include <utility>
#include <vector>

#include "expression.hpp"

struct hash_literal : expression
{
    [[nodiscard]] auto string() const -> std::string override;
    void accept(struct visitor& visitor) const final;

    std::vector<std::pair<expression*, expression*>> pairs;
};
