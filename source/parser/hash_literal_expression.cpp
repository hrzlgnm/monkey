#include <algorithm>
#include <exception>
#include <iterator>
#include <variant>

#include "hash_literal_expression.hpp"

#include <compiler/compiler.hpp>
#include <fmt/format.h>

#include "environment.hpp"
#include "expression.hpp"
#include "object.hpp"

auto hash_literal_expression::string() const -> std::string
{
    std::vector<std::string> strpairs;
    std::transform(pairs.cbegin(),
                   pairs.cend(),
                   std::back_inserter(strpairs),
                   [](const auto& pair) { return fmt::format("{}: {}", pair.first->string(), pair.second->string()); });
    return fmt::format("{{{}}}", fmt::join(strpairs, ", "));
}

auto hash_literal_expression::eval(environment_ptr env) const -> object
{
    hash result;

    for (const auto& [key, value] : pairs) {
        auto eval_key = key->eval(env);
        if (eval_key.is<error>()) {
            return eval_key;
        }
        if (!eval_key.is_hashable()) {
            return make_error("unusable as hash key {}", eval_key.type_name());
        }
        auto eval_val = value->eval(env);
        if (eval_val.is<error>()) {
            return eval_val;
        }
        result.insert({eval_key.hash_key(), std::make_any<object>(eval_val)});
    }
    return {result};
}

auto hash_literal_expression::compile(compiler& comp) const -> void
{
    for (const auto& [key, value] : pairs) {
        key->compile(comp);
        value->compile(comp);
    }
    comp.emit(opcodes::hash, pairs.size() * 2);
}
