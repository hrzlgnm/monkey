#include "compiler/symbol_table.hpp"

#include <gtest/gtest.h>

TEST(symboltable, define)
{
    auto expected = string_map<symbol> {
        {"a", symbol {"a", symbol_scope::global, 0}},
        {"b", symbol {"b", symbol_scope::global, 1}},
    };

    symbol_table global;
    auto a = global.define("a");
    ASSERT_EQ(a, expected["a"]);
    auto b = global.define("b");
    ASSERT_EQ(b, expected["b"]);
}

TEST(symboltable, resolve)
{
    symbol_table global;
    global.define("a");
    global.define("b");

    std::array expecteds {
        symbol {"a", symbol_scope::global, 0},
        symbol {"b", symbol_scope::global, 1},
    };

    for (const auto& expected : expecteds) {
        ASSERT_EQ(global.resolve(expected.name), expected);
    }
}
