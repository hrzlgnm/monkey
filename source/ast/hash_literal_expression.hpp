#pragma once

#include <utility>
#include <vector>

#include "expression.hpp"

struct hash_literal_expression : expression
{
    [[nodiscard]] auto string() const -> std::string override;
    [[nodiscard]] auto eval(environment* env) const -> const object* override;
    auto compile(compiler& comp) const -> void override;

    std::vector<std::pair<expression*, expression*>> pairs;
};
