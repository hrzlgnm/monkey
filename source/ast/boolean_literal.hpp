#pragma once

#include "expression.hpp"

struct boolean_literal : expression
{
    explicit boolean_literal(bool val);
    [[nodiscard]] auto string() const -> std::string override;
    void accept(struct visitor& visitor) const final;

    bool value {};
};
