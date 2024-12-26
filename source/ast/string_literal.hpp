#pragma once

#include "identifier.hpp"

struct string_literal : identifier
{
    using identifier::identifier;
    [[nodiscard]] auto string() const -> std::string override;
    void accept(struct visitor& visitor) const final;
};
