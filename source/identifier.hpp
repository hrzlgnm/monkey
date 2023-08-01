#pragma once

#include "expression.hpp"

struct identifier : expression
{
    using expression::expression;
    identifier(token tokn, std::string val);
    auto string() const -> std::string override;
    auto eval(environment_ptr env) const -> object override;

    std::string value;
};

using identifier_ptr = std::unique_ptr<identifier>;
