#pragma once

#include "expression.hpp"
#include "statements.hpp"

struct if_expression final : expression
{
    [[nodiscard]] auto string() const -> std::string override;
    void accept(struct visitor& visitor) const final;
    auto compile(compiler& comp) const -> void override;
    auto check(symbol_table* symbols) const -> void override;

    expression* condition {};
    block_statement* consequence {};
    block_statement* alternative {};
};
