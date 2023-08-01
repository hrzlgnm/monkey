#pragma once

#include <algorithm>
#include <any>
#include <cstdint>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "callable_expression.hpp"
#include "environment_fwd.hpp"
#include "identifier.hpp"
#include "statements.hpp"

// helper type for std::visit
template<typename... T>
struct overloaded : T...
{
    using T::operator()...;
};
template<class... T>
overloaded(T...) -> overloaded<T...>;

struct nullvalue
{
};

struct error
{
    std::string message;
};

struct object;

using integer_value = std::int64_t;
using string_value = std::string;
using return_value = std::any;

using bound_function = std::pair<const callable_expression*, environment_ptr>;

using value_type = std::variant<nullvalue, bool, integer_value, string_value, return_value, error, bound_function>;

namespace std
{
auto to_string(const value_type&) -> std::string;
}  // namespace std
