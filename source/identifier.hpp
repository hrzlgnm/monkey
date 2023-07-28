#pragma once

#include "expression.hpp"

struct identifier : expression
{
    using expression::expression;
    identifier(token tokn, std::string_view val);
    auto string() const -> std::string override;
    auto eval(environment_ptr env) const -> object override;

    std::string_view value;
};

using identifier_ptr = std::shared_ptr<identifier>;
