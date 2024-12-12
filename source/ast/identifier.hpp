#pragma once

#include "expression.hpp"

struct identifier : expression
{
    explicit identifier(std::string val);
    [[nodiscard]] auto string() const -> std::string override;
    [[nodiscard]] auto eval(environment_ptr env) const -> object_ptr override;
    auto compile(compiler& comp) const -> void override;

    std::string value;
};

using identifier_ptr = identifier*;
