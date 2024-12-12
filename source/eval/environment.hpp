#pragma once

#include <unordered_map>

#include "environment_fwd.hpp"
#include "object.hpp"

struct environment
{
    explicit environment(environment_ptr parent_env = nullptr);

    auto debug() const -> void;
    auto break_cycle() -> void;
    auto get(const std::string& name) const -> object_ptr;
    auto set(const std::string& name, object_ptr val) -> void;

  private:
    std::unordered_map<std::string, object_ptr> m_store;
    environment_ptr m_parent {};
};
