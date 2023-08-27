#pragma once

#include "expression.hpp"

struct boolean : expression
{
    explicit boolean(bool val);
    [[nodiscard]] auto string() const -> std::string override;
    [[nodiscard]] auto eval(environment_ptr env) const -> object_ptr override;
    auto compile(compiler& comp) const -> void override;

    bool value {};
};
