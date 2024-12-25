#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include <fmt/ostream.h>
#include <gc.hpp>

template<typename Value>
using string_map = std::map<std::string, Value, std::less<>>;

enum class symbol_scope : std::uint8_t
{
    global,
    local,
    builtin,
    free,
    function,
    outer,
};
auto operator<<(std::ostream& ost, symbol_scope scope) -> std::ostream&;

template<>
struct fmt::formatter<symbol_scope> : ostream_formatter
{
};

struct symbol_pointer
{
    int level;
    symbol_scope scope;
    size_t index {};
};

auto operator==(const symbol_pointer& lhs, const symbol_pointer& rhs) -> bool;
auto operator<<(std::ostream& ost, const symbol_pointer& ptr) -> std::ostream&;

template<>
struct fmt::formatter<symbol_pointer> : ostream_formatter
{
};

struct symbol
{
    std::string name;
    symbol_scope scope {};
    size_t index {};
    std::optional<symbol_pointer> ptr;

    [[nodiscard]] auto is_local() const -> bool { return scope == symbol_scope::local; }
};

auto operator==(const symbol& lhs, const symbol& rhs) -> bool;
auto operator<<(std::ostream& ost, const symbol& sym) -> std::ostream&;

template<>
struct fmt::formatter<symbol> : ostream_formatter
{
};

struct symbol_table
{
    static auto create() -> symbol_table*;

    static auto create_enclosed(symbol_table* outer, bool inside_loop = false) -> symbol_table*;

    explicit symbol_table(symbol_table* outer = {}, bool inside_loop = {});
    auto define(const std::string& name) -> symbol;
    auto define_outer(const symbol& original, int level) -> symbol;
    auto define_builtin(size_t index, const std::string& name) -> symbol;
    auto define_function_name(const std::string& name) -> symbol;
    auto resolve(const std::string& name, int level = 0) -> std::optional<symbol>;

    [[nodiscard]] auto is_global() const -> bool { return m_outer == nullptr; }

    [[nodiscard]] auto outer() const -> symbol_table* { return m_outer; }

    [[nodiscard]] auto inside_loop() const -> bool { return m_inside_loop; }

    [[nodiscard]] auto num_definitions() const -> size_t { return m_defs; }

    [[nodiscard]] auto free() const -> const std::vector<symbol>&;
    auto debug() const -> void;

  private:
    auto define_free(const symbol& sym) -> symbol;
    symbol_table* m_outer {};
    string_map<symbol> m_store;
    size_t m_defs {};
    std::vector<symbol> m_free;
    bool m_inside_loop {};
};
