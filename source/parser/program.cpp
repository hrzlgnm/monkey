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
        if (result.is_return_value) {
            return result;
        }
        if (result.is<error>()) {
            return result;
        }
    }
    return result;
}

auto program::compile(compiler& comp) const -> void
{
    for (const auto& stmt : statements) {
        stmt->compile(comp);
    }
}
