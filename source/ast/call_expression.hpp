#pragma once

#include <vector>

#include "expression.hpp"

struct call_expression : expression
{
    [[nodiscard]] auto string() const -> std::string override;
    [[nodiscard]] auto eval(environment_ptr env) const -> object_ptr override;
    auto compile(compiler& comp) const -> void override;

    expression_ptr function {};
    std::vector<expression_ptr> arguments;
};
