#include "environment.hpp"

auto environment::get(const std::string_view& name) const -> std::optional<object>
{
    return get(std::string(name));
}

auto environment::get(const std::string& name) const -> std::optional<object>
{
    auto itr = store.find(name);
    if (itr != store.end()) {
        return itr->second;
    }
    return {};
}
auto environment::set(const std::string& name, const object& val) -> object
{
    return store[name] = val;
}

auto environment::set(const std::string_view& name, const object& val) -> object
{
    return set(std::string(name), val);
}

enclosing_environment::enclosing_environment(const weak_environment_ptr& outer_env)
    : outer(outer_env)
{
}

auto enclosing_environment::get(const std::string& name) const -> std::optional<object>
{
    auto itr = store.find(name);
    if (itr == store.end()) {
        auto outer_env = outer.lock();
        return outer_env->get(name);
    }
    return itr->second;
}
