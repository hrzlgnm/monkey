#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string>

template<typename Value>
using string_map = std::map<std::string, Value, std::less<>>;

enum class symbol_scope
{
    global,
};

struct symbol
{
    std::string name;
    symbol_scope scope;
    int index;
};
auto operator==(const symbol& lhs, const symbol& rhs) -> bool;

struct symbol_table
{
    auto define(const std::string& name) -> symbol;
    auto resolve(const std::string& name) -> std::optional<symbol>;

    string_map<symbol> store;
};

using symbol_table_ptr = std::shared_ptr<symbol_table>;
