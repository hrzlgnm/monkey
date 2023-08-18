#include <cstdint>

#include "index_expression.hpp"

#include <compiler/compiler.hpp>

#include "environment.hpp"
#include "object.hpp"

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
        auto max = static_cast<int64_t>(arr.size() - 1);
        if (index < 0 || index > max) {
            return {};
        }
        return arr.at(static_cast<size_t>(index));
    }
    if (evaluated_left.is<hash>()) {
        auto hsh = evaluated_left.as<hash>();
        if (!evaluated_index.is_hashable()) {
            return make_error("unusable as hash key: {}", evaluated_index.type_name());
        }
        if (!hsh.contains(evaluated_index.hash_key())) {
            return {nil};
        }
        return unwrap(hsh.at(evaluated_index.hash_key()));
    }
    return make_error("index operator not supported: {}", evaluated_left.type_name());
}
