#pragma once

#include <vector>

#include "callable_expression.hpp"
#include "statements.hpp"

struct function_expression : callable_expression
{
    function_expression(std::vector<std::string>&& params, const statement* bod);

    [[nodiscard]] auto string() const -> std::string override;
    [[nodiscard]] auto call(environment* closure_env,
                            environment* caller_env,
                            const std::vector<const expression*>& arguments) const -> const object* override;
    auto compile(compiler& comp) const -> void override;
    auto check(analyzer& anlzr, symbol_table* symbols) const -> void override;

    const statement* body {};
    std::string name;
};
