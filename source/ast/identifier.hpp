#pragma once

#include <vector>

#include "expression.hpp"

struct identifier : expression
{
    explicit identifier(std::string val);
    [[nodiscard]] auto string() const -> std::string override;
    void accept(struct visitor& visitor) const override;
    auto compile(compiler& comp) const -> void override;
    auto check(symbol_table* symbols) const -> void override;

    std::string value;
};

using identifiers = std::vector<const identifier*>;
