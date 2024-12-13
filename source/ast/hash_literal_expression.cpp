#include <algorithm>
#include <vector>

#include "hash_literal_expression.hpp"

#include <fmt/format.h>

auto hash_literal_expression::string() const -> std::string
{
    std::vector<std::string> strpairs;
    std::ranges::transform(pairs,
                           std::back_inserter(strpairs),
                           [](const auto& pair)
                           { return fmt::format("{}: {}", pair.first->string(), pair.second->string()); });
    return fmt::format("{{{}}}", fmt::join(strpairs, ", "));
}
