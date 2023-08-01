#include "index_expression.hpp"

#include "environment.hpp"
#include "object.hpp"

auto index_expression::string() const -> std::string
{
    return fmt::format("({}[{}])", left->string(), index->string());
}

auto index_expression::eval(environment_ptr env) const -> object
{
    return {};
}
