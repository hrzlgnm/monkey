#pragma once

#include <lexer/token_type.hpp>

#include "expression.hpp"

struct unary_expression : expression
{
    [[nodiscard]] auto string() const -> std::string override;
    [[nodiscard]] auto eval(environment* env) const -> const object* override;
    auto compile(compiler& comp) const -> void override;
    auto check(analyzer& anlzr, symbol_table* symbols) const -> void override;

    token_type op {};
    expression* right {};
};
