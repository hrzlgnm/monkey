#pragma once

#include <memory>

#include "environment_fwd.hpp"
#include "token.hpp"

struct object;

struct expression
{
    expression() = default;
    expression(const expression&) = default;
    expression(expression&&) = default;
    auto operator=(const expression&) -> expression& = default;
    auto operator=(expression&&) -> expression& = default;
    virtual ~expression() = default;
    explicit expression(token tokn);

    virtual auto string() const -> std::string = 0;
    virtual auto eval(environment_ptr) const -> object = 0;
    token tkn {};
};
using expression_ptr = std::unique_ptr<expression>;
