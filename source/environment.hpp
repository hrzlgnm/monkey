#pragma once

#include <optional>
#include <unordered_map>

#include "object.hpp"

struct environment
{
    environment() = default;
    environment(const environment&) = delete;
    environment(environment&&) = delete;
    auto operator=(const environment&) -> environment& = delete;
    auto operator=(environment&&) -> environment& = delete;
    virtual ~environment() = default;

    auto get(const std::string_view& name) const -> std::optional<object>;
    virtual auto get(const std::string& name) const -> std::optional<object>;
    auto set(const std::string_view& name, const object& val) -> object;
    virtual auto set(const std::string& name, const object& val) -> object;

    std::unordered_map<std::string, object> store;
};

using environment_ptr = std::shared_ptr<environment>;
using weak_environment_ptr = std::weak_ptr<environment>;

struct enclosing_environment : environment
{
    explicit enclosing_environment(const weak_environment_ptr& outer_env);
    auto get(const std::string& name) const -> std::optional<object> override;
    weak_environment_ptr outer;
};
