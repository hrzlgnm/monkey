#include "if_expression.hpp"

#include <fmt/core.h>

auto if_expression::string() const -> std::string
{
    return fmt::format("if {} {} {}",
                       condition->string(),
                       consequence->string(),
                       alternative ? fmt::format("else {}", alternative->string()) : std::string());
}
