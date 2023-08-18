#pragma once

#include "expression.hpp"

struct integer_literal : expression
{
    auto string() const -> std::string override;
    auto eval(environment_ptr env) const -> object override;
    auto compile(compiler& comp) const -> void override;

    int64_t value {};
};
