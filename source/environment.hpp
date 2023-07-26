#pragma once

#include <optional>
#include <unordered_map>

#include "object.hpp"

struct environment
{
    std::unordered_map<std::string, object> store;
    auto get(const std::string_view& value) const -> std::optional<object> { return get(std::string(value)); }
    auto get(const std::string& name) const -> std::optional<object>
    {
        auto itr = store.find(name);
        if (itr != store.end()) {
            return itr->second;
        }
        return {};
    }
    auto set(const std::string& name, const object& val) -> object { return store[name] = val; }
};
