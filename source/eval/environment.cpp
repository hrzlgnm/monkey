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

auto environment::get(std::string_view name) const -> object
{
    for (auto ptr = shared_from_this(); ptr; ptr = ptr->m_parent) {
        if (const auto itr = ptr->m_store.find(std::string {name}); itr != ptr->m_store.end()) {
            return itr->second;
        }
    }
    return nil;
}

auto environment::set(std::string_view name, object&& val) -> void
{
    if (const auto itr = m_store.find(std::string {name}); itr != m_store.end()) {
        itr->second = std::move(val);
    } else {
        m_store.emplace(std::string(name), val);
    }
}
auto environment::set(std::string_view name, const object& val) -> void
{
    m_store[std::string {name}] = val;
}

auto environment::debug() const -> void
{
    for (const auto& [k, v] : m_store) {
        fmt::print("[{}] = {}\n", k, std::to_string(v.value));
    }
}
