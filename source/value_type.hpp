#pragma once

#include <algorithm>
#include <any>
#include <cstdint>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "environment_fwd.hpp"
#include "identifier.hpp"
#include "node.hpp"
#include "statements.hpp"

// helper type for std::visit
template<typename... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

struct nullvalue
{
};

struct error
{
    std::string message;
};

using integer_value = std::int64_t;
using string_value = std::string;
using return_value = std::any;

struct fun
{
    fun() = default;
    fun(const fun&) = delete;
    fun(fun&&) = delete;
    ~fun() = default;
    auto operator=(const fun&) -> fun& = delete;
    auto operator=(fun&&) -> fun& = delete;

    std::vector<identifier_ptr> parameters;
    block_statement_ptr body {};
    environment_ptr env {};
};
using func = std::shared_ptr<fun>;
using value_type = std::variant<nullvalue, bool, integer_value, string_value, return_value, error, func>;

namespace std
{
auto to_string(const value_type&) -> std::string;
}  // namespace std
