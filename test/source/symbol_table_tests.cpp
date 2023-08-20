#include <cmath>

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
        symbol {"c", free, 0},
        symbol {"d", free, 1},
        symbol {"e", local, 0},
        symbol {"f", local, 1},
    };
    for (const auto& expected : expecteds) {
        EXPECT_EQ(nested->resolve(expected.name), expected);
    }
}

TEST(symboltable, defineResolveBuiltin)
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
            EXPECT_EQ(table->resolve(expected.name), expected);
        }
    }
}

TEST(symboltable, resolveFree)
{
    using enum symbol_scope;
    auto globals = symbol_table::create();
    globals->define("a");
    globals->define("b");
    auto first = symbol_table::create_enclosed(globals);
    first->define("c");
    first->define("d");
    auto nested = symbol_table::create_enclosed(first);
    nested->define("e");
    nested->define("f");

    ASSERT_EQ(nested->resolve("c")->scope, free);
    ASSERT_EQ(nested->resolve("d")->scope, free);
    ASSERT_EQ(nested->free().size(), 2);
    ASSERT_EQ(nested->free().at(0).name, "c");
    ASSERT_EQ(nested->free().at(1).name, "d");
}

TEST(symboltable, defineAndResolveFunctonName)
{
    using enum symbol_scope;
    auto globals = symbol_table::create();
    globals->define_function_name("a");

    auto expected = symbol {"a", function, 0};

    auto actual = globals->resolve("a");
    ASSERT_TRUE(actual.has_value());
    ASSERT_EQ(expected, actual.value());
}

TEST(symboltable, shadowFunctionNames)
{
    using enum symbol_scope;
    auto globals = symbol_table::create();
    globals->define_function_name("a");
    globals->define("a");

    auto expected = symbol {"a", global, 0};
    auto resolved = globals->resolve("a");
    ASSERT_TRUE(resolved.has_value());
    ASSERT_EQ(resolved.value(), expected);
}
// NOLINTEND(*-identifier-length)
// NOLINTEND(*-magic-numbers)
