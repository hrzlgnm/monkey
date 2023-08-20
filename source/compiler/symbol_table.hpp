#pragma once

#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

template<typename Value>
using string_map = std::map<std::string, Value, std::less<>>;

enum class symbol_scope
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
    symbol_scope scope;
    size_t index;
    inline auto is_local() const -> bool { return scope == symbol_scope::local; }
};
auto operator==(const symbol& lhs, const symbol& rhs) -> bool;
auto operator<<(std::ostream& ost, const symbol& sym) -> std::ostream&;

struct symbol_table;
using symbol_table_ptr = std::shared_ptr<symbol_table>;

struct symbol_table : std::enable_shared_from_this<symbol_table>
{
    static inline auto create() -> symbol_table_ptr { return std::make_shared<symbol_table>(); }
    static inline auto create_enclosed(symbol_table_ptr outer) -> symbol_table_ptr
    {
        return std::make_shared<symbol_table>(std::move(outer));
    }
    explicit symbol_table(symbol_table_ptr outer = {});
    auto define(const std::string& name) -> symbol;
    auto define_builtin(size_t index, const std::string& name) -> symbol;
    auto define_function_name(const std::string& name) -> symbol;
    auto resolve(const std::string& name) -> std::optional<symbol>;
    inline auto is_global() const -> bool { return !m_parent; }
    inline auto parent() const -> symbol_table_ptr { return m_parent; }
    inline auto num_definitions() const -> size_t { return m_defs; }
    auto free() const -> std::vector<symbol>;

  private:
    auto define_free(const symbol& sym) -> symbol;
    symbol_table_ptr m_parent;
    string_map<symbol> m_store;
    size_t m_defs {};
    std::vector<symbol> m_free;
};
