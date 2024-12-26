#pragma once

#include "identifier.hpp"
#include "statements.hpp"

struct function_literal final : expression
{
    function_literal(identifiers&& params, const block_statement* bod);

    [[nodiscard]] auto string() const -> std::string override;
    void accept(struct visitor& visitor) const final;

    std::string name;
    identifiers parameters;
    const block_statement* body {};
};
