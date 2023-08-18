#include <string>

#include "environment.hpp"

#include "object.hpp"

environment::environment(environment_ptr parent_env)
    : parent(std::move(parent_env))
{
}

auto environment::break_cycle() -> void
{
    store.clear();
}

auto environment::get(std::string_view name) const -> object
{
    for (auto ptr = shared_from_this(); ptr; ptr = ptr->parent) {
        if (const auto itr = ptr->store.find(std::string {name}); itr != ptr->store.end()) {
            return itr->second;
        }
    }
    return {nil_value {}};
}

auto environment::set(std::string_view name, object&& val) -> void
{
    if (const auto itr = store.find(std::string {name}); itr != store.end()) {
        itr->second = std::move(val);
    } else {
        store.emplace(std::string(name), val);
    }
}
auto environment::set(std::string_view name, const object& val) -> void
{
    store[std::string {name}] = val;
}

auto debug_env(const environment_ptr& env) -> void
{
    for (const auto& [k, v] : env->store) {
        fmt::print("[{}] = {}\n", k, std::to_string(v.value));
    }
}
