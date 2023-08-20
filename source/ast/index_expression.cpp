#include "index_expression.hpp"

#include <fmt/core.h>

auto index_expression::string() const -> std::string
{
    return fmt::format("({}[{}])", left->string(), index->string());
}
