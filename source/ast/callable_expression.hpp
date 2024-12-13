#pragma once
#include <vector>

#include "expression.hpp"

struct callable_expression : expression
{
    explicit callable_expression(std::vector<std::string> params);

    [[nodiscard]] auto eval(environment* env) const -> const object* override;
    [[nodiscard]] auto string() const -> std::string override;
    [[nodiscard]] virtual auto call(environment* closure_env,
                                    environment* caller_env,
                                    const std::vector<const expression*>& arguments) const -> const object* = 0;

    std::vector<std::string> parameters;
};
