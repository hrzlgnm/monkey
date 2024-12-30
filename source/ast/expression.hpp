#pragma once

#include <string>
#include <vector>

#include <lexer/location.hpp>

struct expression
{
    explicit expression(location loc)
        : l {loc}
    {
    }

    virtual ~expression() = default;
    expression(const expression&) = delete;
    expression(expression&&) = delete;
    auto operator=(const expression&) -> expression& = delete;
    auto operator=(expression&&) -> expression& = delete;

    [[nodiscard]] virtual auto string() const -> std::string = 0;
    virtual void accept(struct visitor& visitor) const = 0;

    [[nodiscard]] auto loc() const { return l; }

    location l;
};

using expressions = std::vector<const expression*>;
