#include "program.hpp"

#include <fmt/format.h>

#include "object.hpp"
#include "util.hpp"

auto program::string() const -> std::string
{
    return fmt::format("{}", join(statements));
}

auto program::eval(environment_ptr env) const -> object
{
    object result;
    for (const auto& statement : statements) {
        result = statement->eval(env);
        if (result.is<return_value>()) {
            return unwrap_return_value(result);
        }
        if (result.is<error>()) {
            return result;
        }
    }
    return result;
}
