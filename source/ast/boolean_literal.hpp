#pragma once

#include "expression.hpp"

struct boolean_literal : expression
{
    explicit boolean_literal(bool val);
    [[nodiscard]] auto string() const -> std::string override;
    [[nodiscard]] auto eval(environment* env) const -> const object* override;
    auto compile(compiler& comp) const -> void override;

    auto check(symbol_table* /*symbols*/) const -> void override {}

    bool value {};
};
