#pragma once
#include <vector>

#include "expression.hpp"
#include "identifier.hpp"

using statement = expression;

struct let_statement : statement
{
    [[nodiscard]] auto string() const -> std::string override;
    [[nodiscard]] auto eval(environment* env) const -> const object* override;
    auto compile(compiler& comp) const -> void override;

    const identifier* name {};
    const expression* value {};
};

struct return_statement : statement
{
    [[nodiscard]] auto string() const -> std::string override;
    [[nodiscard]] auto eval(environment* env) const -> const object* override;
    auto compile(compiler& comp) const -> void override;

    const expression* value {};
};

struct expression_statement : statement
{
    [[nodiscard]] auto string() const -> std::string override;
    [[nodiscard]] auto eval(environment* env) const -> const object* override;
    auto compile(compiler& comp) const -> void override;

    const expression* expr {};
};

struct block_statement : statement
{
    [[nodiscard]] auto string() const -> std::string override;
    [[nodiscard]] auto eval(environment* env) const -> const object* override;
    auto compile(compiler& comp) const -> void override;

    std::vector<const statement*> statements;
};
