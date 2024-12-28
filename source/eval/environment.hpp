#pragma once

#include <unordered_map>

struct object;

struct environment final
{
    explicit environment(environment* outer_env = nullptr);

    auto get(const std::string& name) const -> const object*;
    auto set(const std::string& name, const object* val) -> void;
    auto reassign(const std::string& name, const object* val) -> const object*;

    void debug() const;

    std::unordered_map<std::string, const object*> store;
    environment* outer {};
};
