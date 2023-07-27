#pragma once

#include "expression.hpp"
#include "statements.hpp"

struct function_literal : expression
{
    using expression::expression;
    auto string() const -> std::string override;
    auto eval(environment_ptr env) const -> object override;

    std::vector<identifier_ptr> parameters;
    block_statement_ptr body {};
};
