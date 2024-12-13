#pragma once

#include <unordered_map>

struct object;

struct environment
{
    explicit environment(environment* parent_env = nullptr);

    auto debug() const -> void;
    auto get(const std::string& name) const -> const object*;
    auto set(const std::string& name, const object* val) -> void;

  private:
    std::unordered_map<std::string, const object*> m_store;
    environment* m_parent {};
};
