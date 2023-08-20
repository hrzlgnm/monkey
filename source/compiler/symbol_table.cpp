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
        case free:
            return ost << "free";
        case function:
            return ost << "function";
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

auto symbol_table::define_function_name(const std::string& name) -> symbol
{
    return m_store[name] = symbol {
               .name = name,
               .scope = symbol_scope::function,
               .index = 0,
           };
}
auto symbol_table::resolve(const std::string& name) -> std::optional<symbol>
{
    using enum symbol_scope;
    if (m_store.contains(name)) {
        return m_store[name];
    }
    if (m_parent) {
        auto maybe_symbol = m_parent->resolve(name);
        if (!maybe_symbol.has_value()) {
            return maybe_symbol;
        }
        auto symbol = maybe_symbol.value();
        if (symbol.scope == global || symbol.scope == builtin) {
            return symbol;
        }
        return define_free(symbol);
    }
    return {};
}
auto symbol_table::free() const -> std::vector<symbol>
{
    return m_free;
}

auto symbol_table::define_free(const symbol& sym) -> symbol
{
    m_free.push_back(sym);
    return m_store[sym.name] = symbol {.name = sym.name, .scope = symbol_scope::free, .index = m_free.size() - 1};
}
