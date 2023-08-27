#pragma once

#include <utility>
#include <vector>

#include "expression.hpp"

struct hash_literal_expression : expression
{
    [[nodiscard]] auto string() const -> std::string override;
    [[nodiscard]] auto eval(environment_ptr env) const -> object_ptr override;
    auto compile(compiler& comp) const -> void override;

    std::vector<std::pair<expression_ptr, expression_ptr>> pairs;
};
