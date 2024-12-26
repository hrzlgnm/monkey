#pragma once

#include <string>

struct compiler;
struct environment;
struct object;
struct symbol_table;
struct analyzer;

struct expression
{
    expression() = default;
    virtual ~expression() = default;
    expression(const expression&) = delete;
    expression(expression&&) = delete;
    auto operator=(const expression&) -> expression& = delete;
    auto operator=(expression&&) -> expression& = delete;

    [[nodiscard]] virtual auto string() const -> std::string = 0;
    [[nodiscard]] virtual auto eval(environment*) const -> const object* = 0;
    virtual inline auto compile(compiler& /*comp*/) const -> void = 0;
    virtual auto check(analyzer& anlzr, symbol_table* symbols) const -> void = 0;
};
