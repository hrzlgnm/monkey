#pragma once

#include <iostream>
#include <string_view>

#include "token_type.hpp"

struct token
{
    token_type type;
    std::string_view literal;
    auto operator==(const token& other) const -> bool;
};

auto operator<<(std::ostream& ostream, const token& token) -> std::ostream&;
