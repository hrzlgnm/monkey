#include <string>

#include "assign_expression.hpp"

#include <fmt/format.h>

auto assign_expression::string() const -> std::string
{
    return fmt::format("{} = {};", name->string(), value->string());
}
