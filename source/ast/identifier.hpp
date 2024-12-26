#pragma once

#include "expression.hpp"

struct identifier : expression
{
    explicit identifier(std::string val);
    [[nodiscard]] auto string() const -> std::string override;
    [[nodiscard]] auto eval(environment* env) const -> const object* override;
    auto compile(compiler& comp) const -> void override;
    auto check(analyzer& anlzr, symbol_table* symbols) const -> void override;

    std::string value;
};
