#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include <chungus.hpp>
#include <fmt/ostream.h>

template<typename Value>
using string_map = std::map<std::string, Value, std::less<>>;

enum class symbol_scope : std::uint8_t
{
    global,
    local,
    builtin,
    free,
    function,
};
auto operator<<(std::ostream& ost, symbol_scope scope) -> std::ostream&;

struct symbol
{
    std::string name;
    symbol_scope scope {};
    size_t index {};

    [[nodiscard]] auto is_local() const -> bool { return scope == symbol_scope::local; }
};

auto operator==(const symbol& lhs, const symbol& rhs) -> bool;
auto operator<<(std::ostream& ost, const symbol& sym) -> std::ostream&;

struct symbol_table;

struct symbol_table : std::enable_shared_from_this<symbol_table>
{
    static auto create() -> symbol_table* { return make<symbol_table>(); }

    static auto create_enclosed(symbol_table* outer) -> symbol_table* { return make<symbol_table>(outer); }

    explicit symbol_table(symbol_table* outer = {});
    auto define(const std::string& name) -> symbol;
    auto define_builtin(size_t index, const std::string& name) -> symbol;
    auto define_function_name(const std::string& name) -> symbol;
    auto resolve(const std::string& name) -> std::optional<symbol>;

    auto is_global() const -> bool { return m_parent == nullptr; }

    auto parent() const -> symbol_table* { return m_parent; }

    auto num_definitions() const -> size_t { return m_defs; }

    auto free() const -> std::vector<symbol>;

  private:
    auto define_free(const symbol& sym) -> symbol;
    symbol_table* m_parent {};
    string_map<symbol> m_store;
    size_t m_defs {};
    std::vector<symbol> m_free;
};
