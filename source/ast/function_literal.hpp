#pragma once
#include <utility>

#include <lexer/location.hpp>

#include "identifier.hpp"
#include "statements.hpp"

struct function_literal final : expression
{
    function_literal(identifiers&& params, const block_statement* bod, location loc)
        : expression {loc}
        , parameters {std::move(params)}
        , body {bod} {};

    [[nodiscard]] auto string() const -> std::string final;
    void accept(struct visitor& visitor) const final;

    std::string name;
    identifiers parameters;
    const block_statement* body {};
};
