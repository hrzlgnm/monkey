#pragma once

#include "identifier.hpp"

struct string_literal : identifier
{
    using identifier::identifier;
    [[nodiscard]] auto string() const -> std::string override;
    [[nodiscard]] auto eval(environment* env) const -> const object* override;
    auto compile(compiler& comp) const -> void override;

    auto check(symbol_table* /*symbols*/) const -> void override {}
};
