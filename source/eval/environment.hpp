#pragma once

#include <unordered_map>

struct object;

struct environment
{
    explicit environment(environment* outer_env = nullptr);

    auto debug() const -> void;
    auto get(const std::string& name) const -> const object*;
    auto set(const std::string& name, const object* val) -> void;

    std::unordered_map<std::string, const object*> store;
    environment* outer {};
};
