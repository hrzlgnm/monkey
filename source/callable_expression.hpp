#pragma once
#include <vector>

#include "environment_fwd.hpp"
#include "expression.hpp"

struct callable_expression
    : expression
    , std::enable_shared_from_this<callable_expression>
{
    ~callable_expression() override = default;

    explicit callable_expression(std::vector<std::string>&& parameters);

    auto eval(environment_ptr env) const -> object override;
    auto string() const -> std::string override;
    virtual auto call(environment_ptr closure_env,
                      environment_ptr caller_env,
                      const std::vector<expression_ptr>& arguments) const -> object = 0;

    std::vector<std::string> parameters;
};
