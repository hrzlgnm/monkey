#pragma once

#include <algorithm>
#include <memory>
#include <string_view>
#include <type_traits>
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
};

struct statement : public node
{
    using node::node;
};

struct expression : public node
{
    using node::node;
};

struct program : public node
{
    using node::node;
    inline auto token_literal() const -> std::string_view override
    {
        if (statements.empty()) {
            return "";
        }
        return statements[0]->token_literal();
    }

    std::vector<std::unique_ptr<statement>> statements {};
};

struct identifier : public node
{
    using node::node;
    inline auto token_literal() const -> std::string_view override
    {
        return tkn.literal;
    }
    token tkn {};
    std::string_view value;
};

struct let_statement : public statement
{
    using statement::statement;

    inline auto token_literal() const -> std::string_view override
    {
        return tkn.literal;
    }
    token tkn {};
    std::unique_ptr<identifier> name {};
    std::unique_ptr<expression> value {};
};

struct return_statement : public statement
{
    using statement::statement;
    inline auto token_literal() const -> std::string_view override
    {
        return tkn.literal;
    }
    token tkn {};
    std::unique_ptr<expression> return_value {};
};
