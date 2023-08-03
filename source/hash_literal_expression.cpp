#include <algorithm>
#include <iterator>

#include "hash_literal_expression.hpp"

#include "environment.hpp"

auto hash_literal_expression::string() const -> std::string
{
    std::vector<std::string> strpairs;
    std::transform(pairs.cbegin(),
                   pairs.cend(),
                   std::back_inserter(strpairs),
                   [](const auto& pair) { return fmt::format("{}: {}", pair.first->string(), pair.second->string()); });
    return fmt::format("{{{}}}", fmt::join(strpairs, ", "));
}

auto hash_literal_expression::eval(environment_ptr env) const -> object
{
    return {};
}
