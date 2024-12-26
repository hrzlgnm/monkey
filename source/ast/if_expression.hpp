#pragma once

#include "expression.hpp"
#include "statements.hpp"

struct if_expression final : expression
{
    [[nodiscard]] auto string() const -> std::string override;
    void accept(struct visitor& visitor) const final;

    expression* condition {};
    block_statement* consequence {};
    block_statement* alternative {};
};
