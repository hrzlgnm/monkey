#include <string>

#include "environment.hpp"

#include "object.hpp"

environment::environment(environment_ptr parent_env)
    : m_parent(parent_env)
{
    if (m_parent == this) {
        abort();
    }
}

auto environment::break_cycle() -> void
{
    m_store.clear();
}

auto environment::get(const std::string& name) const -> object_ptr
{
    for (const auto* ptr = this; ptr != nullptr; ptr = ptr->m_parent) {
        if (const auto itr = ptr->m_store.find(name); itr != ptr->m_store.end()) {
            return itr->second;
        }
    }
    return &null_obj;
}

auto environment::set(const std::string& name, object_ptr val) -> void
{
    m_store[name] = val;
}

auto environment::debug() const -> void
{
    for (const auto& [k, v] : m_store) {
        fmt::print("[{}] = {}\n", k, v->inspect());
    }
}
