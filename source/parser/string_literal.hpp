#pragma once

#include "identifier.hpp"

struct string_literal : identifier
{
    using identifier::identifier;
    auto string() const -> std::string override;
    auto eval(environment_ptr env) const -> object override;
    auto compile(compiler& comp) const -> void override;
};
