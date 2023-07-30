#pragma once

#include <vector>

#include "callable_expression.hpp"
#include "environment_fwd.hpp"
#include "expression.hpp"
#include "statements.hpp"

struct function_expression : callable_expression
{
    function_expression(std::vector<std::string>&& parameters, statement_ptr&& body);

    auto string() const -> std::string override;
    auto call(environment_ptr closure_env,
              environment_ptr caller_env,
              const std::vector<expression_ptr>& arguments) const -> object override;

    inline auto shared_from_this() -> std::shared_ptr<function_expression>
    {
        return std::static_pointer_cast<function_expression>(callable_expression::shared_from_this());
    }
    environment_ptr parent_env;
    statement_ptr body;
};
