#pragma once

#include "expression.hpp"

struct boolean_literal final : expression
{
    explicit boolean_literal(bool val);
    [[nodiscard]] auto string() const -> std::string final;
    void accept(struct visitor& visitor) const final;

    bool value {};
};
