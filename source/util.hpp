#pragma once

#include <algorithm>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/format.h>

#include "environment_fwd.hpp"

template<typename T>
auto join(const std::vector<std::unique_ptr<T>>& nodes, std::string_view sep = {}) -> std::string
{
    auto strs = std::vector<std::string>();
    std::transform(
        nodes.cbegin(), nodes.cend(), std::back_inserter(strs), [](const auto& node) { return node->string(); });
    return fmt::format("{}", fmt::join(strs.cbegin(), strs.cend(), sep));
}

auto debug_env(const environment_ptr& env) -> void;

template<typename T>
void unused(T /*unused*/)
{
}

template<typename V>
using string_map = std::map<std::string, V, std::less<>>;
