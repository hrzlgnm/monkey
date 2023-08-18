#pragma once

#include <vector>

#include "callable_expression.hpp"
#include "statements.hpp"

struct function_expression : callable_expression
{
    function_expression(std::vector<std::string>&& parameters, statement_ptr&& body);

    auto string() const -> std::string override;
    auto call(environment_ptr closure_env,
              environment_ptr caller_env,
              const std::vector<expression_ptr>& arguments) const -> object override;
    // auto compile(compiler& comp) const -> void override;

    environment_ptr parent_env;
    statement_ptr body;
};
