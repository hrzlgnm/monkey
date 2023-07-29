#include "environment.hpp"

#include "environment_fwd.hpp"

environment::environment(environment_ptr parent_env)
    : parent(std::move(parent_env))
{
}

static const object nil {nullvalue {}};

auto environment::get(std::string_view name) const -> object
{
    for (auto ptr = shared_from_this(); ptr; ptr = ptr->parent) {
        if (const auto itr = ptr->store.find(std::string {name}); itr != ptr->store.end()) {
            return itr->second;
        }
    }
    return nil;
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
