#pragma once

#include <string>

#include "expression.hpp"

struct string_literal final : expression
{
    explicit string_literal(std::string val);

    [[nodiscard]] auto string() const -> std::string final;
    void accept(struct visitor& visitor) const final;

    std::string value;
};
