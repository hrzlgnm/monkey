#pragma once

#include <memory>
#include <unordered_map>

#include "environment_fwd.hpp"
#include "object.hpp"

struct environment : std::enable_shared_from_this<environment>
{
    explicit environment(environment_ptr parent_env = {});
    environment(const environment&) = delete;
    environment(environment&&) = delete;
    auto operator=(const environment&) -> environment& = delete;
    auto operator=(environment&&) -> environment& = delete;
    virtual ~environment() = default;

    auto break_cycle() -> void;
    auto get(std::string_view name) const -> object;
    auto set(std::string_view name, object&& val) -> void;
    auto set(std::string_view name, const object& val) -> void;

    std::unordered_map<std::string, object> store;
    environment_ptr parent;
};
