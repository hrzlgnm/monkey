#pragma once

#include <algorithm>
#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
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
    inline auto token_literal() const -> std::string_view override
    {
        if (statements.empty()) {
            return "";
        }
        return statements[0]->token_literal();
    }

    auto string() const -> std::string override
    {
        std::stringstream strm;
        for (const auto& statement : statements) {
            strm << statement->string();
        }
        return strm.str();
    }
    std::vector<std::unique_ptr<statement>> statements {};
};

struct identifier : expression
{
    using expression::expression;
    inline identifier(token tokn, std::string_view val)
        : tkn {tokn}
        , value {val}
    {
    }
    inline auto token_literal() const -> std::string_view override
    {
        return tkn.literal;
    }
    inline auto string() const -> std::string override
    {
        return {value.data(), value.size()};
    }
    token tkn {};
    std::string_view value;
};

struct let_statement : statement
{
    using statement::statement;

    inline auto token_literal() const -> std::string_view override
    {
        return tkn.literal;
    }
    inline auto string() const -> std::string override
    {
        std::stringstream strm;
        strm << token_literal() << " " << name->string() << " = ";
        if (value) {
            strm << value->string();
        }
        strm << ';';
        return strm.str();
    }
    token tkn {};
    std::unique_ptr<identifier> name {};
    std::unique_ptr<expression> value {};
};

struct return_statement : statement
{
    using statement::statement;
    inline auto token_literal() const -> std::string_view override
    {
        return tkn.literal;
    }
    inline auto string() const -> std::string override
    {
        std::stringstream strm;
        strm << token_literal() << " ";
        if (return_value) {
            strm << return_value->string();
        }
        strm << ';';
        return strm.str();
    }
    token tkn {};
    std::unique_ptr<expression> return_value {};
};

struct expression_statement : statement
{
    using statement::statement;
    inline auto token_literal() const -> std::string_view override
    {
        return tkn.literal;
    }
    inline auto string() const -> std::string override
    {
        if (expr) {
            return expr->string();
        }
        return {};
    }
    token tkn {};
    std::unique_ptr<expression> expr {};
};

struct integer_literal : expression
{
    using expression::expression;
    inline auto token_literal() const -> std::string_view override
    {
        return tkn.literal;
    }
    inline auto string() const -> std::string override
    {
        return std::string {tkn.literal};
    }

    token tkn {};
    int64_t value {};
};
