#pragma once
#include <ostream>
#include <string_view>

#include <fmt/ostream.h>

struct location final
{
    std::string_view filename;
    std::string::size_type line;
    std::string::size_type column;
    auto operator==(const location& other) const -> bool = default;
};

auto operator<<(std::ostream& os, const location& l) -> std::ostream&;

template<>
struct fmt::formatter<location> : ostream_formatter
{
};
