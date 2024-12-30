#pragma once

#include <string>

#include "expression.hpp"

struct null_literal final : expression
{
    using expression::expression;
    [[nodiscard]] auto string() const -> std::string final;
    void accept(struct visitor& visitor) const final;
};
