#pragma once
#include <cstdint>

#include "expression.hpp"

struct integer_literal final : expression
{
    [[nodiscard]] auto string() const -> std::string final;
    void accept(struct visitor& visitor) const final;

    std::int64_t value {};
};
