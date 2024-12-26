#pragma once

#include "expression.hpp"

struct program final : expression
{
    [[nodiscard]] auto string() const -> std::string override;
    void accept(struct visitor& visitor) const final;

    expressions statements;
};
