#include <string>

#include "statements.hpp"

#include <fmt/format.h>

#include "util.hpp"

auto let_statement::string() const -> std::string
{
    return fmt::format("let {} = {};", name->string(), (value != nullptr) ? value->string() : std::string());
}

auto return_statement::string() const -> std::string
{
    return fmt::format("return {};", (value != nullptr) ? value->string() : std::string());
}

auto expression_statement::string() const -> std::string
{
    if (expr != nullptr) {
        return expr->string();
    }
    return {};
}

auto block_statement::string() const -> std::string
{
    return join(statements);
}
