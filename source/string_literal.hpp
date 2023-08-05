#pragma once

#include "expression.hpp"

struct string_literal : expression
{
    auto string() const -> std::string override;
    auto eval(environment_ptr env) const -> object override;

    std::string value {};
};
