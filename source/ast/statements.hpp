#pragma once

#include "expression.hpp"
#include "identifier.hpp"

using statement = expression;

struct let_statement final : statement
{
    [[nodiscard]] auto string() const -> std::string final;
    void accept(struct visitor& visitor) const final;

    const identifier* name {};
    const expression* value {};
};

struct return_statement final : statement

{
    [[nodiscard]] auto string() const -> std::string final;
    void accept(struct visitor& visitor) const final;

    const expression* value {};
};

struct break_statement final : statement
{
    [[nodiscard]] auto string() const -> std::string final;
    void accept(struct visitor& visitor) const final;
};

struct continue_statement final : statement
{
    [[nodiscard]] auto string() const -> std::string final;
    void accept(struct visitor& visitor) const final;
};

struct expression_statement final : statement
{
    [[nodiscard]] auto string() const -> std::string final;
    void accept(struct visitor& visitor) const final;

    const expression* expr {};
};

struct block_statement final : statement
{
    [[nodiscard]] auto string() const -> std::string final;
    void accept(struct visitor& visitor) const final;

    expressions statements;
};

struct while_statement final : statement
{
    [[nodiscard]] auto string() const -> std::string final;
    void accept(struct visitor& visitor) const final;

    expression* condition {};
    block_statement* body {};
};
