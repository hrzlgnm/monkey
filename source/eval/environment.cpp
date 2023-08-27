#include <string>

#include "environment.hpp"

#include "object.hpp"

environment::environment(environment_ptr parent_env)
    : m_parent(std::move(parent_env))
{
}

auto environment::break_cycle() -> void
{
    m_store.clear();
}

auto environment::get(const std::string& name) const -> object_ptr
{
    for (auto ptr = shared_from_this(); ptr; ptr = ptr->m_parent) {
        if (const auto itr = ptr->m_store.find(name); itr != ptr->m_store.end()) {
            return itr->second;
        }
    }
    return null;
}

auto environment::set(const std::string& name, const object_ptr& val) -> void
{
    m_store[name] = val;
}

auto environment::debug() const -> void
{
    for (const auto& [k, v] : m_store) {
        fmt::print("[{}] = {}\n", k, v->inspect());
    }
}
