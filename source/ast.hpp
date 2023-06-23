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
    explicit statement(token tokn);
    token tkn {};
};
using statement_ptr = std::unique_ptr<statement>;

struct expression : node
{
    using node::node;
    explicit expression(token tokn);
    token tkn {};
};

using expression_ptr = std::unique_ptr<expression>;

struct program : node
{
    using node::node;
    auto token_literal() const -> std::string_view override;

    auto string() const -> std::string override;

    std::vector<std::unique_ptr<statement>> statements {};
};
using program_ptr = std::unique_ptr<program>;

struct identifier : expression
{
    using expression::expression;
    identifier(token tokn, std::string_view val);
    auto token_literal() const -> std::string_view override;

    auto string() const -> std::string override;

    std::string_view value;
};

using identifier_ptr = std::unique_ptr<identifier>;

struct let_statement : statement
{
    using statement::statement;

    auto token_literal() const -> std::string_view override;

    auto string() const -> std::string override;

    std::unique_ptr<identifier> name {};
    std::unique_ptr<expression> value {};
};

struct return_statement : statement
{
    using statement::statement;
    auto token_literal() const -> std::string_view override;

    auto string() const -> std::string override;

    std::unique_ptr<expression> return_value {};
};

struct expression_statement : statement
{
    using statement::statement;
    auto token_literal() const -> std::string_view override;

    auto string() const -> std::string override;

    std::unique_ptr<expression> expr {};
};

struct boolean : expression
{
    boolean(token tokn, bool val);
    auto token_literal() const -> std::string_view override;

    auto string() const -> std::string override;

    bool value {};
};

struct integer_literal : expression
{
    using expression::expression;
    auto token_literal() const -> std::string_view override;

    auto string() const -> std::string override;

    int64_t value {};
};

struct prefix_expression : expression
{
    using expression::expression;
    auto token_literal() const -> std::string_view override;
    auto string() const -> std::string override;

    std::string op {};
    expression_ptr right {};
};

struct infix_expression : expression
{
    using expression::expression;
    auto token_literal() const -> std::string_view override;
    auto string() const -> std::string override;

    expression_ptr left {};
    std::string op {};
    expression_ptr right {};
};

struct block_statement : statement
{
    using statement::statement;
    auto token_literal() const -> std::string_view override;
    auto string() const -> std::string override;

    std::vector<statement_ptr> statements {};
};

using block_statement_ptr = std::unique_ptr<block_statement>;

struct if_expression : expression
{
    using expression::expression;
    auto token_literal() const -> std::string_view override;
    auto string() const -> std::string override;

    expression_ptr condition {};
    block_statement_ptr consequence {};
    block_statement_ptr alternative {};
};

struct function_literal : expression
{
    using expression::expression;
    auto token_literal() const -> std::string_view override;
    auto string() const -> std::string override;

    std::vector<identifier_ptr> parameters;
    block_statement_ptr body;
};
