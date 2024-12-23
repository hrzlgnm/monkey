#include <string>

#include "environment.hpp"

#include <fmt/base.h>
#include <object/object.hpp>

environment::environment(environment* outer_env)
    : outer(outer_env)
{
}

auto environment::get(const std::string& name) const -> const object*
{
    for (const auto* ptr = this; ptr != nullptr; ptr = ptr->outer) {
        if (const auto itr = ptr->store.find(name); itr != ptr->store.end()) {
            return itr->second;
        }
    }
    return null_object();
}

auto environment::set(const std::string& name, const object* val) -> void
{
    store[name] = val;
}

auto environment::reassign(const std::string& name, const object* val) -> const object*
{
    if (const auto itr = store.find(name); itr == store.end() && outer != nullptr) {
        return outer->reassign(name, val);
    }
    set(name, val);
    return val;
}

auto environment::debug() const -> void
{
    for (const auto& [k, v] : store) {
        fmt::print("[{}] = {}\n", k, v->inspect());
    }
}
