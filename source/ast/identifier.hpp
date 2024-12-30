#pragma once

#include <utility>
#include <vector>

#include <lexer/location.hpp>

#include "expression.hpp"

struct identifier final : expression
{
    explicit identifier(std::string val, location loc)
        : expression {loc}
        , value {std::move(val)}
    {
    }

    [[nodiscard]] auto string() const -> std::string override;
    void accept(struct visitor& visitor) const override;

    std::string value;
};

using identifiers = std::vector<const identifier*>;
