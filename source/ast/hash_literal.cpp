#include <algorithm>
#include <iterator>
#include <string>
#include <vector>

#include "hash_literal.hpp"

#include <fmt/format.h>
#include <fmt/ranges.h>

#include "visitor.hpp"

auto hash_literal::string() const -> std::string
{
    std::vector<std::string> strpairs;
    std::transform(pairs.cbegin(),
                   pairs.cend(),
                   std::back_inserter(strpairs),
                   [](const auto& pair) { return fmt::format("{}: {}", pair.first->string(), pair.second->string()); });
    return fmt::format("{{{}}}", fmt::join(strpairs, ", "));
}

void hash_literal::accept(visitor& visitor) const
{
    visitor.visit(*this);
}
