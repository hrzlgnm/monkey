#pragma once

#include "identifier.hpp"

struct string_literal : identifier
{
    using identifier::identifier;
    [[nodiscard]] auto string() const -> std::string override;
    [[nodiscard]] auto eval(environment_ptr env) const -> object_ptr override;
    auto compile(compiler& comp) const -> void override;
};
