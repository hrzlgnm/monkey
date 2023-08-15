#pragma once

#include <algorithm>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/format.h>

#include "environment_fwd.hpp"

template<typename Expression>
auto join(const std::vector<std::unique_ptr<Expression>>& nodes, std::string_view sep = {}) -> std::string
{
    auto strs = std::vector<std::string>();
    std::transform(
        nodes.cbegin(), nodes.cend(), std::back_inserter(strs), [](const auto& node) { return node->string(); });
    return fmt::format("{}", fmt::join(strs.cbegin(), strs.cend(), sep));
}

auto debug_env(const environment_ptr& env) -> void;

template<typename Value>
using string_map = std::map<std::string, Value, std::less<>>;

[[nodiscard]] auto read_uint16_big_endian(const std::vector<uint8_t>& bytes, size_t offset) -> uint16_t;
void write_uint16_big_endian(std::vector<uint8_t>& bytes, size_t offset, uint16_t value);
