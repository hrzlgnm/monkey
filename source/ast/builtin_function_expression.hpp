#pragma once

#include <functional>

#include <eval/object.hpp>

#include "callable_expression.hpp"

struct builtin_function_expression : callable_expression
{
    builtin_function_expression(std::string&& name,
                                std::vector<std::string>&& params,
                                std::function<object_ptr(std::vector<object_ptr>&& arguments)>&& bod);

    [[nodiscard]] auto call(environment_ptr closure_env,
                            environment_ptr caller_env,
                            const std::vector<expression_ptr>& arguments) const -> object_ptr override;
    [[nodiscard]] auto string() const -> std::string override;
    auto compile(compiler& comp) const -> void override;

    static const std::vector<builtin_function_expression> builtins;

    std::string name;
    std::function<object_ptr(array_object::array&& arguments)> body;
};
