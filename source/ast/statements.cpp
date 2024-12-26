#include <string>

#include "statements.hpp"

#include <fmt/format.h>

#include "util.hpp"
#include "visitor.hpp"

auto let_statement::string() const -> std::string
{
    return fmt::format("let {} = {};", name->string(), (value != nullptr) ? value->string() : std::string());
}

void let_statement::accept(visitor& visitor) const
{
    visitor.visit(*this);
}

auto return_statement::string() const -> std::string
{
    return fmt::format("return {};", (value != nullptr) ? value->string() : std::string());
}

void return_statement::accept(visitor& visitor) const
{
    visitor.visit(*this);
}

auto break_statement::string() const -> std::string
{
    return "break";
}

void break_statement::accept(visitor& visitor) const
{
    visitor.visit(*this);
}

auto continue_statement::string() const -> std::string
{
    return "continue";
}

void continue_statement::accept(visitor& visitor) const
{
    visitor.visit(*this);
}

auto expression_statement::string() const -> std::string
{
    if (expr != nullptr) {
        return expr->string();
    }
    return {};
}

void expression_statement::accept(visitor& visitor) const
{
    visitor.visit(*this);
}

auto block_statement::string() const -> std::string
{
    return join(statements);
}

void block_statement::accept(visitor& visitor) const
{
    visitor.visit(*this);
}

auto while_statement::string() const -> std::string
{
    return fmt::format("while {} {}", condition->string(), body->string());
}

void while_statement::accept(visitor& visitor) const
{
    visitor.visit(*this);
}
