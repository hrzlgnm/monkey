#pragma once

#include <vector>

#include "expression.hpp"
#include "statements.hpp"

struct program : expression
{
    auto string() const -> std::string override;
    auto eval(environment_ptr env) const -> object override;
    auto compile(compiler& comp) const -> void override;

    std::vector<statement_ptr> statements {};
};
using program_ptr = std::unique_ptr<program>;
