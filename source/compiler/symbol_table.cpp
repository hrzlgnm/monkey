#include "symbol_table.hpp"

#include <fmt/ostream.h>

auto operator==(const symbol& lhs, const symbol& rhs) -> bool
{
    return lhs.name == rhs.name && lhs.scope == rhs.scope && lhs.index == rhs.index;
}

auto operator<<(std::ostream& ost, symbol_scope scope) -> std::ostream&
{
    using enum symbol_scope;
    switch (scope) {
        case global:
            return ost << "global";
        case local:
            return ost << "local";
        case builtin:
            return ost << "builtin";
    }
    return ost;
}

auto operator<<(std::ostream& ost, const symbol& sym) -> std::ostream&
{
    return ost << fmt::format("symbol{{{}, {}, {}}}", sym.name, sym.scope, sym.index);
}

symbol_table::symbol_table(symbol_table_ptr outer)
    : m_parent(std::move(outer))
{
}

auto symbol_table::define(const std::string& name) -> symbol
{
    using enum symbol_scope;
    return m_store[name] = symbol {
               .name = name,
               .scope = m_parent ? local : global,
               .index = m_defs++,
           };
}

auto symbol_table::define_builtin(size_t index, const std::string& name) -> symbol
{
    return m_store[name] = symbol {
               .name = name,
               .scope = symbol_scope::builtin,
               .index = index,
           };
}

auto symbol_table::resolve(const std::string& name) -> std::optional<symbol>
{
    for (auto tbl = shared_from_this(); tbl; tbl = tbl->m_parent) {
        if (const auto itr = tbl->m_store.find(name); itr != tbl->m_store.end()) {
            return itr->second;
        }
    }
    return {};
}
