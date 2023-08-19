#include "compiler/symbol_table.hpp"

#include <gtest/gtest.h>

// NOLINTBEGIN(*-magic-numbers)
// NOLINTBEGIN(*-identifier-length)
TEST(symboltable, define)
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
    EXPECT_EQ(a, expected["a"]);
    auto b = globals->define("b");
    EXPECT_EQ(b, expected["b"]);

    auto first = symbol_table::create_enclosed(globals);
    auto c = first->define("c");
    EXPECT_EQ(c, expected["c"]);
    auto d = first->define("d");
    EXPECT_EQ(d, expected["d"]);

    auto second = symbol_table::create_enclosed(first);
    auto e = second->define("e");
    EXPECT_EQ(e, expected["e"]);
    auto f = second->define("f");
    EXPECT_EQ(f, expected["f"]);
}

TEST(symboltable, resolve)
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
        EXPECT_EQ(globals->resolve(expected.name), expected);
    }
}

TEST(symboltable, resolveLocal)
{
    auto globals = symbol_table::create();
    globals->define("a");
    globals->define("b");

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
        EXPECT_EQ(locals->resolve(expected.name), expected);
    }
}

TEST(symboltable, resolveNestedLocals)
{
    auto globals = symbol_table::create();
    globals->define("a");
    globals->define("b");

    auto first = symbol_table::create_enclosed(globals);
    first->define("c");
    first->define("d");

    auto nested = symbol_table::create_enclosed(first);
    nested->define("e");
    nested->define("f");

    using enum symbol_scope;
    std::array expecteds {
        symbol {"a", global, 0},
        symbol {"b", global, 1},
        symbol {"c", local, 0},
        symbol {"d", local, 1},
        symbol {"e", local, 0},
        symbol {"f", local, 1},
    };
    for (const auto& expected : expecteds) {
        EXPECT_EQ(nested->resolve(expected.name), expected);
    }
}
// NOLINTEND(*-identifier-length)
// NOLINTEND(*-magic-numbers)
