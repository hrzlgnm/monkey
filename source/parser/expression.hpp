#pragma once

#include <memory>

#include "environment_fwd.hpp"

struct object;
struct compiler;

struct expression
{
    expression() = default;
    expression(const expression&) = default;
    expression(expression&&) = default;
    auto operator=(const expression&) -> expression& = default;
    auto operator=(expression&&) -> expression& = default;
    virtual ~expression() = default;

    virtual auto string() const -> std::string = 0;
    virtual auto eval(environment_ptr) const -> object = 0;
    virtual auto compile(compiler& comp) const -> void { __builtin_unreachable(); }
};
using expression_ptr = std::unique_ptr<expression>;
