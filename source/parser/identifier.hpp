#pragma once

#include "expression.hpp"

struct identifier : expression
{
    explicit identifier(std::string val);
    auto string() const -> std::string override;
    auto eval(environment_ptr env) const -> object override;
    // auto compile(compiler& comp) const -> void override;

    std::string value;
};

using identifier_ptr = std::unique_ptr<identifier>;
