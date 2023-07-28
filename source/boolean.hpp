#pragma once

#include "expression.hpp"

struct boolean : expression
{
    boolean(token tokn, bool val);
    auto string() const -> std::string override;
    auto eval(environment_ptr env) const -> object override;

    bool value {};
};
