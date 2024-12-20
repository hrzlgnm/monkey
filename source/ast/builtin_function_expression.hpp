#pragma once

#include <functional>
#include <string>
#include <vector>

#include <object/object.hpp>

#include "callable_expression.hpp"

struct builtin_function_expression : callable_expression
{
    builtin_function_expression(std::string name,
                                std::vector<std::string> params,
                                std::function<const object*(std::vector<const object*>&& arguments)> bod);

    [[nodiscard]] auto call(environment* closure_env,
                            environment* caller_env,
                            const std::vector<const expression*>& arguments) const -> const object* override;
    [[nodiscard]] auto string() const -> std::string override;
    auto compile(compiler& comp) const -> void override;

    static const std::vector<const builtin_function_expression*> builtins;

    std::string name;
    std::function<const object*(array_object::value_type&& arguments)> body;
};
