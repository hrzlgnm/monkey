#pragma once
#include <vector>

#include "expression.hpp"
#include "identifier.hpp"
#include "token.hpp"

using statement = expression;
using statement_ptr = expression_ptr;

struct let_statement : statement
{
    using statement::statement;
    auto string() const -> std::string override;
    auto eval(environment_ptr env) const -> object override;

    identifier_ptr name {};
    expression_ptr value {};
};

struct return_statement : statement
{
    using statement::statement;
    auto string() const -> std::string override;
    auto eval(environment_ptr env) const -> object override;
    expression_ptr value {};
};

struct expression_statement : statement
{
    using statement::statement;
    auto string() const -> std::string override;
    auto eval(environment_ptr env) const -> object override;

    expression_ptr expr {};
};

struct block_statement : statement
{
    using statement::statement;
    auto string() const -> std::string override;
    auto eval(environment_ptr env) const -> object override;

    std::vector<statement_ptr> statements {};
};

using block_statement_ptr = std::unique_ptr<block_statement>;
