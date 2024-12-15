#include <algorithm>
#include <string>
#include <vector>

#include "hash_literal_expression.hpp"

#include <fmt/format.h>
#include <fmt/ranges.h>

auto hash_literal_expression::string() const -> std::string
{
    std::vector<std::string> strpairs;
    std::transform(pairs.cbegin(),
                   pairs.cend(),
                   std::back_inserter(strpairs),
                   [](const auto& pair) { return fmt::format("{}: {}", pair.first->string(), pair.second->string()); });
    return fmt::format("{{{}}}", fmt::join(strpairs, ", "));
}
