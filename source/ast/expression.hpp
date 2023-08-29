#pragma once

#include <memory>

#include <eval/environment_fwd.hpp>
#include <eval/object.hpp>

struct compiler;

struct expression
{
    expression() = default;
    expression(const expression&) = default;
    expression(expression&&) = default;
    auto operator=(const expression&) -> expression& = default;
    auto operator=(expression&&) -> expression& = default;
    virtual ~expression() = default;

    [[nodiscard]] virtual auto string() const -> std::string = 0;
    [[nodiscard]] virtual auto eval(environment_ptr) const -> object_ptr = 0;
    virtual inline auto compile(compiler& /*comp*/) const -> void = 0;
};

using expression_ptr = std::unique_ptr<expression>;
