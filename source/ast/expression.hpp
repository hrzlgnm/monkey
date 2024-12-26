#pragma once

#include <string>
#include <vector>

struct compiler;
struct environment;
struct object;
struct symbol_table;

struct expression
{
    expression() = default;
    virtual ~expression() = default;
    expression(const expression&) = delete;
    expression(expression&&) = delete;
    auto operator=(const expression&) -> expression& = delete;
    auto operator=(expression&&) -> expression& = delete;

    [[nodiscard]] virtual auto string() const -> std::string = 0;
    virtual void accept(struct visitor& visitor) const = 0;
    virtual inline auto compile(compiler& /*comp*/) const -> void = 0;
    virtual auto check(symbol_table* symbols) const -> void = 0;
};

using expressions = std::vector<const expression*>;
