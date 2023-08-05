#pragma once

#include "expression.hpp"

struct boolean : expression
{
    explicit boolean(bool val);
    auto string() const -> std::string override;
    auto eval(environment_ptr env) const -> object override;

    bool value {};
};
