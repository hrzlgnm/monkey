#include "binary_expression.hpp"

#include <fmt/core.h>
#include <lexer/token_type.hpp>

auto binary_expression::string() const -> std::string
{
    return fmt::format("({} {} {})", left->string(), op, right->string());
}
