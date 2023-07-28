#include "program.hpp"

#include <fmt/format.h>

#include "object.hpp"
#include "util.hpp"

auto program::token_literal() const -> std::string_view
{
    if (statements.empty()) {
        return "";
    }
    return statements[0]->token_literal();
}

auto program::string() const -> std::string
{
    return join(statements);
}

auto program::eval(environment_ptr env) const -> object
{
    object result;
    for (const auto& statement : statements) {
        result = statement->eval(env);
        if (result.is<return_value>()) {
            return std::any_cast<object>(result.as<return_value>());
        }
        if (result.is<error>()) {
            return result;
        }
    }
    return result;
}
