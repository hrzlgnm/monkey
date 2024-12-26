#pragma once

#include <vector>

#include "expression.hpp"

struct identifier : expression
{
    explicit identifier(std::string val);
    [[nodiscard]] auto string() const -> std::string override;
    void accept(struct visitor& visitor) const override;

    std::string value;
};

using identifiers = std::vector<const identifier*>;
