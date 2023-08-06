#pragma once

#include "expression.hpp"
#include "statements.hpp"

struct if_expression : expression
{
    auto string() const -> std::string override;
    auto eval(environment_ptr env) const -> object override;
    // auto compile(compiler& comp) const -> void override;

    expression_ptr condition {};
    block_statement_ptr consequence {};
    block_statement_ptr alternative {};
};
