#pragma once
#include <cstdint>

#include "expression.hpp"

struct integer_literal final : expression
{
    [[nodiscard]] auto string() const -> std::string override;
    void accept(struct visitor& visitor) const final;
    auto compile(compiler& comp) const -> void override;

    auto check(symbol_table* /*symbols*/) const -> void override {}

    std::int64_t value {};
};
