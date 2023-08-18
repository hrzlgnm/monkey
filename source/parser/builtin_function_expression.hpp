#pragma once

#include <functional>

#include "callable_expression.hpp"
#include "object.hpp"

struct builtin_function_expression : callable_expression
{
    builtin_function_expression(std::string&& name,
                                std::vector<std::string>&& params,
                                std::function<object(array&& arguments)>&& bod);

    auto call(environment_ptr closure_env,
              environment_ptr caller_env,
              const std::vector<expression_ptr>& arguments) const -> object override;
    auto string() const -> std::string override;
    // auto compile(compiler& comp) const -> void override;

    static const std::vector<builtin_function_expression> builtins;

    std::string name;
    std::function<object(array&& arguments)> body;
};
