#include "symbol_table.hpp"

auto operator==(const symbol& lhs, const symbol& rhs) -> bool
{
    return lhs.name == rhs.name && lhs.scope == rhs.scope && lhs.index == rhs.index;
}

auto symbol_table::define(const std::string& name) -> symbol
{
    return store[name] = symbol {
               .name = name,
               .scope = symbol_scope::global,
               .index = store.size(),
           };
}

auto symbol_table::resolve(const std::string& name) -> std::optional<symbol>
{
    if (store.contains(name)) {
        return store[name];
    }
    return {};
}
