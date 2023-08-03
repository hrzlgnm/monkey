#include "index_expression.hpp"

#include "environment.hpp"
#include "object.hpp"
#include "value_type.hpp"

auto index_expression::string() const -> std::string
{
    return fmt::format("({}[{}])", left->string(), index->string());
}

auto index_expression::eval(environment_ptr env) const -> object
{
    auto evaluated_left = left->eval(env);
    if (evaluated_left.is<error>()) {
        return evaluated_left;
    }
    auto evaluated_index = index->eval(env);
    if (evaluated_index.is<error>()) {
        return evaluated_index;
    }
    if (evaluated_left.is<array>() && evaluated_index.is<integer_value>()) {
        auto arr = evaluated_left.as<array>();
        auto index = evaluated_index.as<integer_value>();
        auto max = arr.size() - 1;
        if (index < 0 || index > max) {
            return {};
        }
        return *arr[index];
    }
    return make_error("index operator not supported: {}", evaluated_left.type_name());
}
