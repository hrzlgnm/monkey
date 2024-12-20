#pragma once

#include <object/object.hpp>

struct compiler;

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
};
