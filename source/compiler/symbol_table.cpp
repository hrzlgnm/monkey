#include <array>
#include <ostream>
#include <string>

#include "symbol_table.hpp"

#include <doctest/doctest.h>
#include <fmt/format.h>
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

template<>
struct fmt::formatter<symbol_scope> : ostream_formatter
{
};

auto operator<<(std::ostream& ost, const symbol& sym) -> std::ostream&
{
    return ost << fmt::format("symbol{{{}, {}, {}}}", sym.name, sym.scope, sym.index);
}

symbol_table::symbol_table(symbol_table* outer)
    : m_parent(outer)
{
}

auto symbol_table::define(const std::string& name) -> symbol
{
    using enum symbol_scope;
    return m_store[name] = symbol {
               .name = name,
               .scope = (m_parent != nullptr) ? local : global,
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
    if (m_parent != nullptr) {
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

namespace
{
// NOLINTBEGIN(*)
TEST_SUITE_BEGIN("symbol table");

TEST_CASE("define")
{
    using enum symbol_scope;
    auto expected = string_map<symbol> {
        {"a", symbol {"a", global, 0}},
        {"b", symbol {"b", global, 1}},
        {"c", symbol {"c", local, 0}},
        {"d", symbol {"d", local, 1}},
        {"e", symbol {"e", local, 0}},
        {"f", symbol {"f", local, 1}},
    };

    auto globals = symbol_table::create();
    auto a = globals->define("a");
    CHECK_EQ(a, expected["a"]);
    auto b = globals->define("b");
    CHECK_EQ(b, expected["b"]);

    auto first = symbol_table::create_enclosed(globals);
    auto c = first->define("c");
    CHECK_EQ(c, expected["c"]);
    auto d = first->define("d");
    CHECK_EQ(d, expected["d"]);

    auto second = symbol_table::create_enclosed(first);
    auto e = second->define("e");
    CHECK_EQ(e, expected["e"]);
    auto f = second->define("f");
    CHECK_EQ(f, expected["f"]);
}

TEST_CASE("resolve")
{
    auto globals = symbol_table::create();
    globals->define("a");
    globals->define("b");

    using enum symbol_scope;
    std::array expecteds {
        symbol {"a", global, 0},
        symbol {"b", global, 1},
    };

    for (const auto& expected : expecteds) {
        CHECK_EQ(globals->resolve(expected.name), expected);
    }

    SUBCASE("resolveLocal")
    {
        auto locals = symbol_table::create_enclosed(globals);
        locals->define("c");
        locals->define("d");

        using enum symbol_scope;
        std::array expecteds {
            symbol {"a", global, 0},
            symbol {"b", global, 1},
            symbol {"c", local, 0},
            symbol {"d", local, 1},
        };
        for (const auto& expected : expecteds) {
            CHECK_EQ(locals->resolve(expected.name), expected);
        }

        SUBCASE("resolveNestedLocals")
        {
            auto nested = symbol_table::create_enclosed(locals);
            nested->define("e");
            nested->define("f");

            using enum symbol_scope;
            std::array expecteds {
                symbol {"a", global, 0},
                symbol {"b", global, 1},
                symbol {"c", free, 0},
                symbol {"d", free, 1},
                symbol {"e", local, 0},
                symbol {"f", local, 1},
            };
            for (const auto& expected : expecteds) {
                CHECK_EQ(nested->resolve(expected.name), expected);
            }

            SUBCASE("resolveFree")
            {
                REQUIRE_EQ(nested->free().size(), 2);
                CHECK_EQ(nested->free()[0].name, "c");
                CHECK_EQ(nested->free()[1].name, "d");
            }
        }
    }
}

TEST_CASE("defineResolveBuiltin")
{
    using enum symbol_scope;
    std::array expecteds {
        symbol {"a", builtin, 0},
        symbol {"c", builtin, 1},
        symbol {"e", builtin, 2},
        symbol {"f", builtin, 3},
    };

    auto globals = symbol_table::create();
    auto first = symbol_table::create_enclosed(globals);
    auto nested = symbol_table::create_enclosed(first);
    for (size_t i = 0; const auto& expected : expecteds) {
        globals->define_builtin(i, expected.name);
        i++;
    }

    for (const auto& table : {globals, first, nested}) {
        for (const auto& expected : expecteds) {
            CHECK_EQ(table->resolve(expected.name), expected);
        }
    }
}

TEST_CASE("defineAndResolveFunctonName")
{
    using enum symbol_scope;
    auto globals = symbol_table::create();
    globals->define_function_name("a");

    auto expected = symbol {"a", function, 0};

    auto actual = globals->resolve("a");
    REQUIRE(actual.has_value());
    REQUIRE_EQ(expected, actual.value());
}

TEST_CASE("shadowFunctionNames")
{
    using enum symbol_scope;
    auto globals = symbol_table::create();
    globals->define_function_name("a");
    globals->define("a");

    auto expected = symbol {"a", global, 0};
    auto resolved = globals->resolve("a");
    REQUIRE(resolved.has_value());
    REQUIRE_EQ(resolved.value(), expected);
}

TEST_SUITE_END();
// NOLINTEND(*)
}  // namespace
