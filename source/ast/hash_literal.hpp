#pragma once

#include <utility>
#include <vector>

#include "expression.hpp"

struct hash_literal : expression
{
    [[nodiscard]] auto string() const -> std::string override;
    void accept(struct visitor& visitor) const final;
    auto compile(compiler& comp) const -> void override;

    std::vector<std::pair<expression*, expression*>> pairs;
};
