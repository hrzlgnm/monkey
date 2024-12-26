#include <string>
#include <utility>

#include "function_literal.hpp"

#include <fmt/format.h>

#include "identifier.hpp"
#include "statements.hpp"
#include "util.hpp"
#include "visitor.hpp"

function_literal::function_literal(identifiers&& params, const block_statement* bod)
    : parameters {std::move(params)}
    , body {bod}
{
}

auto function_literal::string() const -> std::string
{
    return fmt::format("fn({}) {{ {}; }}", join(parameters, ", "), body->string());
}

void function_literal::accept(visitor& visitor) const
{
    visitor.visit(*this);
}
