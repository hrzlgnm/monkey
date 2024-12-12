#pragma once

#include <vector>

#include "expression.hpp"
#include "statements.hpp"

struct program : expression
{
    [[nodiscard]] auto string() const -> std::string override;
    [[nodiscard]] auto eval(environment_ptr env) const -> object_ptr override;
    auto compile(compiler& comp) const -> void override;

    std::vector<statement_ptr> statements;
};

using program_ptr = program*;
