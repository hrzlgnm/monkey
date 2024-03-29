#pragma once
#include <vector>

#include "expression.hpp"

struct callable_expression : expression
{
    callable_expression(callable_expression&&) = delete;
    auto operator=(callable_expression&&) -> callable_expression& = default;
    callable_expression(const callable_expression&) = default;
    auto operator=(const callable_expression&) -> callable_expression& = default;
    explicit callable_expression(std::vector<std::string>&& params);
    ~callable_expression() override = default;

    [[nodiscard]] auto eval(environment_ptr env) const -> object_ptr override;
    [[nodiscard]] auto string() const -> std::string override;
    [[nodiscard]] virtual auto call(environment_ptr closure_env,
                                    environment_ptr caller_env,
                                    const std::vector<expression_ptr>& arguments) const -> object_ptr = 0;

    std::vector<std::string> parameters;
};
