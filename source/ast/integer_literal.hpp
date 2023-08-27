#pragma once

#include "expression.hpp"

struct integer_literal : expression
{
    [[nodiscard]] auto string() const -> std::string override;
    [[nodiscard]] auto eval(environment_ptr env) const -> object_ptr override;
    auto compile(compiler& comp) const -> void override;

    int64_t value {};
};
