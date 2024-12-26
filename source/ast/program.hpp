#pragma once

#include <vector>

#include "expression.hpp"
#include "statements.hpp"

struct program : expression
{
    [[nodiscard]] auto string() const -> std::string override;
    [[nodiscard]] auto eval(environment* env) const -> const object* override;
    auto compile(compiler& comp) const -> void override;
    auto check(analyzer& anlzr, symbol_table* symbols) const -> void override;

    std::vector<const statement*> statements;
};
