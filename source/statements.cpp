#include <algorithm>
#include <iterator>
#include <vector>

#include "statements.hpp"

#include <fmt/core.h>
#include <fmt/format.h>

#include "environment.hpp"
#include "object.hpp"
#include "util.hpp"

auto let_statement::string() const -> std::string
{
    return fmt::format("{} {} = {};", tkn.literal, name->string(), value ? value->string() : std::string());
}

auto let_statement::eval(environment_ptr env) const -> object
{
    auto val = value->eval(env);
    if (val.is<error>()) {
        return val;
    }
    env->set(name->value, std::move(val));
    return {};
}

auto return_statement::string() const -> std::string
{
    return fmt::format("{} {};", tkn.literal, value ? value->string() : std::string());
}

auto return_statement::eval(environment_ptr env) const -> object
{
    if (value) {
        auto evaluated = value->eval(env);
        if (evaluated.is<error>()) {
            return evaluated;
        }
        return object {std::make_any<object>(evaluated)};
    }
    return {};
}

auto expression_statement::string() const -> std::string
{
    if (expr) {
        return expr->string();
    }
    return {};
}

auto expression_statement::eval(environment_ptr env) const -> object
{
    if (expr) {
        return expr->eval(env);
    }
    return {};
}

auto block_statement::string() const -> std::string
{
    return join(statements);
}

auto block_statement::eval(environment_ptr env) const -> object
{
    object result;
    for (const auto& stmt : statements) {
        result = stmt->eval(env);
        if (result.is<return_value>() || result.is<error>()) {
            return result;
        }
    }
    return result;
}
