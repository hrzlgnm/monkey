#include <string>

#include "program.hpp"

#include <fmt/format.h>

#include "util.hpp"

auto program::string() const -> std::string
{
    return fmt::format("{}", join(statements));
}
