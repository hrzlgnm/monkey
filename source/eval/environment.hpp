#pragma once

#include <memory>
#include <unordered_map>

#include "environment_fwd.hpp"
#include "object.hpp"

struct environment : std::enable_shared_from_this<environment>
{
    explicit environment(environment_ptr parent_env = {});

    auto debug() const -> void;
    auto break_cycle() -> void;
    auto get(const std::string& name) const -> object_ptr;
    auto set(const std::string& name, const object_ptr& val) -> void;

  private:
    std::unordered_map<std::string, object_ptr> m_store;
    environment_ptr m_parent;
};
