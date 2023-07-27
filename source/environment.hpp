#pragma once

#include <optional>
#include <unordered_map>

#include "object.hpp"

struct environment
{
    environment() = default;
    environment(const environment&) = default;
    environment(environment&&) = delete;
    auto operator=(const environment&) -> environment& = default;
    auto operator=(environment&&) -> environment& = delete;
    virtual ~environment() = default;
    std::unordered_map<std::string, object> store;
    auto get(const std::string_view& name) const -> std::optional<object>;
    virtual auto get(const std::string& name) const -> std::optional<object>;
    auto set(const std::string_view& name, const object& val) -> object;
    virtual auto set(const std::string& name, const object& val) -> object;
};

using environment_ptr = std::shared_ptr<environment>;

struct enclosing_environment : environment
{
    explicit enclosing_environment(environment_ptr outer_env);
    auto get(const std::string& name) const -> std::optional<object> override;
    environment_ptr outer;
};
