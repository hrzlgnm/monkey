#pragma once

#include <utility>
#include <vector>

#include "expression.hpp"

struct hash_literal final : expression
{
    using expression::expression;
    [[nodiscard]] auto string() const -> std::string final;
    void accept(struct visitor& visitor) const final;

    std::vector<std::pair<expression*, expression*>> pairs;
};
