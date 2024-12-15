#include <string>

#include "if_expression.hpp"

#include <fmt/format.h>

auto if_expression::string() const -> std::string
{
    return fmt::format("if {} {} {}",
                       condition->string(),
                       consequence->string(),
                       (alternative != nullptr) ? fmt::format("else {}", alternative->string()) : std::string());
}
