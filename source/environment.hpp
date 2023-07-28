#pragma once

#include <optional>
#include <unordered_map>

#include "environment_fwd.hpp"
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

struct enclosing_environment : environment
{
    explicit enclosing_environment(weak_environment_ptr outer_env);
    auto get(const std::string& name) const -> std::optional<object> override;
    weak_environment_ptr outer;
};
