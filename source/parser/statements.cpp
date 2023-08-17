#include <algorithm>
#include <iterator>
#include <vector>

#include "statements.hpp"

#include <fmt/core.h>
#include <fmt/format.h>

#include "code.hpp"
#include "compiler.hpp"
#include "environment.hpp"
#include "object.hpp"
#include "util.hpp"

auto let_statement::string() const -> std::string
{
    return fmt::format("let {} = {};", name->string(), value ? value->string() : std::string());
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

auto let_statement::compile(compiler& comp) const -> void
{
    value->compile(comp);
    auto symbol = comp.symbols->define(name->value);
    comp.emit(opcodes::set_global, {symbol.index});
}

auto return_statement::string() const -> std::string
{
    return fmt::format("return {};", value ? value->string() : std::string());
}

auto return_statement::eval(environment_ptr env) const -> object
{
    if (value) {
        auto evaluated = value->eval(env);
        if (evaluated.is<error>()) {
            return evaluated;
        }
        evaluated.is_return_value = true;
        return evaluated;
    }
    return {};
}

auto return_statement::compile(compiler& comp) const -> void
{
    value->compile(comp);
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

auto expression_statement::compile(compiler& comp) const -> void
{
    expr->compile(comp);
    comp.emit(opcodes::pop);
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
        if (result.is_return_value || result.is<error>()) {
            return result;
        }
    }
    return result;
}

auto block_statement::compile(compiler& comp) const -> void
{
    for (const auto& stmt : statements) {
        stmt->compile(comp);
    }
}
