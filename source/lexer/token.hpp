#pragma once

#include <iostream>
#include <string_view>

#include "location.hpp"
#include "token_type.hpp"

struct token final
{
    token_type type;
    std::string_view literal;
    location loc;
    [[nodiscard]] auto with_loc(location loc) const -> token;
    auto operator==(const token& other) const -> bool;
};

auto operator<<(std::ostream& ostream, const token& token) -> std::ostream&;
