#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "token.hpp"
#include "token_type.hpp"

struct node
{
    node() = default;
    node(const node&) = delete;
    node(node&&) = delete;
    auto operator=(const node&) -> node& = delete;
    auto operator=(node&&) -> node& = delete;
    virtual ~node() = default;
    virtual auto token_literal() const -> std::string_view = 0;
    virtual auto string() const -> std::string = 0;
};

struct statement : node
{
    using node::node;
};
using statement_ptr = std::unique_ptr<statement>;

struct expression : node
{
    using node::node;
};

using expression_ptr = std::unique_ptr<expression>;

struct program : node
{
    using node::node;
    auto token_literal() const -> std::string_view override;

    auto string() const -> std::string override;

    std::vector<std::unique_ptr<statement>> statements {};
};

struct identifier : expression
{
    using expression::expression;
    identifier(token tokn, std::string_view val);
    auto token_literal() const -> std::string_view override;

    auto string() const -> std::string override;

    token tkn {};
    std::string_view value;
};

struct let_statement : statement
{
    using statement::statement;

    auto token_literal() const -> std::string_view override;

    auto string() const -> std::string override;

    token tkn {};
    std::unique_ptr<identifier> name {};
    std::unique_ptr<expression> value {};
};

struct return_statement : statement
{
    using statement::statement;
    auto token_literal() const -> std::string_view override;

    auto string() const -> std::string override;

    token tkn {};
    std::unique_ptr<expression> return_value {};
};

struct expression_statement : statement
{
    using statement::statement;
    auto token_literal() const -> std::string_view override;

    auto string() const -> std::string override;

    token tkn {};
    std::unique_ptr<expression> expr {};
};

struct integer_literal : expression
{
    using expression::expression;
    auto token_literal() const -> std::string_view override;

    auto string() const -> std::string override;

    token tkn {};
    int64_t value {};
};

struct prefix_expression : expression
{
    using expression::expression;
    auto token_literal() const -> std::string_view override;
    auto string() const -> std::string override;

    token tkn {};
    std::string op {};
    expression_ptr right {};
};

struct infix_expression : expression
{
    using expression::expression;
    auto token_literal() const -> std::string_view override;
    auto string() const -> std::string override;

    token tkn {};
    expression_ptr left {};
    std::string op {};
    expression_ptr right {};
};
