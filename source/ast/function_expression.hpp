#pragma once

#include <vector>

#include "callable_expression.hpp"
#include "statements.hpp"

struct function_expression : callable_expression
{
    function_expression(std::vector<std::string>&& parameters, statement_ptr body);

    [[nodiscard]] auto string() const -> std::string override;
    [[nodiscard]] auto call(environment_ptr closure_env,
                            environment_ptr caller_env,
                            const std::vector<expression_ptr>& arguments) const -> object_ptr override;
    auto compile(compiler& comp) const -> void override;

    statement_ptr body;
    std::string name;
};
