#include <string>

#include "environment.hpp"

#include "object.hpp"

environment::environment(environment* parent_env)
    : m_parent(parent_env)
{
    if (m_parent == this) {
        abort();
    }
}

auto environment::get(const std::string& name) const -> const object*
{
    for (const auto* ptr = this; ptr != nullptr; ptr = ptr->m_parent) {
        if (const auto itr = ptr->m_store.find(name); itr != ptr->m_store.end()) {
            return itr->second;
        }
    }
    return native_null();
}

auto environment::set(const std::string& name, const object* val) -> void
{
    m_store[name] = val;
}

auto environment::debug() const -> void
{
    for (const auto& [k, v] : m_store) {
        fmt::print("[{}] = {}\n", k, v->inspect());
    }
}
