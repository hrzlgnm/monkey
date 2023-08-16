#pragma once

#include <memory>
#include <stdexcept>

#include <fmt/core.h>

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
    /// TODO: make this pure virtual again
    virtual inline auto compile(compiler& comp) const -> void
    {
        throw std::runtime_error(fmt::format("compile for {} is not implemented yet", typeid(*this).name() + 1));
    }
};
using expression_ptr = std::unique_ptr<expression>;
