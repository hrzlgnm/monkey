#include <algorithm>
#include <cstdint>
#include <iterator>

#include <compiler/code.hpp>
#include <compiler/compiler.hpp>
#include <gtest/gtest.h>
#include <parser/parser.hpp>

#include "testutils.hpp"

// NOLINTBEGIN(*-magic-numbers)

TEST(compiler, compilerScopes)
{
    using enum opcodes;
    auto cmplr = compiler::create();
    ASSERT_EQ(cmplr.scope_index, 0);

    cmplr.emit(mul);
    cmplr.enter_scope();
    ASSERT_EQ(cmplr.scope_index, 1);
    EXPECT_FALSE(cmplr.symbols->is_global());

    cmplr.emit(sub);
    ASSERT_EQ(cmplr.scopes[cmplr.scope_index].instrs.size(), 1);
    ASSERT_EQ(cmplr.scopes[cmplr.scope_index].last_instr.opcode, sub);

    cmplr.leave_scope();
    ASSERT_EQ(cmplr.scope_index, 0);
    EXPECT_TRUE(cmplr.symbols->is_global());

    cmplr.emit(add);
    ASSERT_EQ(cmplr.scopes[cmplr.scope_index].instrs.size(), 2);
    ASSERT_EQ(cmplr.scopes[cmplr.scope_index].last_instr.opcode, add);
    ASSERT_EQ(cmplr.scopes[cmplr.scope_index].previous_instr.opcode, mul);
}

using expected_value = std::variant<int64_t, std::string, std::vector<instructions>>;

struct ctc
{
    std::string_view input;
    std::vector<expected_value> expected_constants;
    std::vector<instructions> expected_instructions;
};

auto assert_instructions(const std::vector<instructions>& instructions, const ::instructions& code)
{
    auto flattened = flatten(instructions);
    EXPECT_EQ(flattened.size(), code.size());
    EXPECT_EQ(flattened, code) << "expected: \n" << to_string(flattened) << "got: \n" << to_string(code);
}

auto assert_constants(const std::vector<expected_value>& expecteds, const constants& consts)
{
    EXPECT_EQ(expecteds.size(), consts.size());
    for (size_t idx = 0; const auto& expected : expecteds) {
        const auto& actual = consts.at(idx);
        std::visit(
            overloaded {
                [&](const int64_t val) { EXPECT_EQ(val, actual.as<integer_type>()); },
                [&](const std::string& val) { EXPECT_EQ(val, actual.as<string_type>()); },
                [&](const std::vector<instructions>& instrs)
                { assert_instructions(instrs, actual.as<compiled_function>().instrs); },
            },
            expected);
        idx++;
    }
}

template<size_t N>
auto run(std::array<ctc, N>&& tests)
{
    for (const auto& [input, constants, instructions] : tests) {
        auto [prgrm, _] = assert_program(input);
        auto cmplr = compiler::create();
        cmplr.compile(prgrm);
        assert_instructions(instructions, cmplr.current_instrs());
        assert_constants(constants, *cmplr.consts);
    }
}

TEST(compiler, integerArithmetics)
{
    using enum opcodes;
    std::array tests {
        ctc {
            "1 + 2",
            {{1}, {2}},
            {make(constant, 0), make(constant, 1), make(add), make(pop)},
        },
        ctc {
            "1; 2",
            {{1}, {2}},
            {make(constant, 0), make(pop), make(constant, 1), make(pop)},
        },
        ctc {
            "1 - 2",
            {{1}, {2}},
            {make(constant, 0), make(constant, 1), make(sub), make(pop)},
        },
        ctc {
            "1 * 2",
            {{1}, {2}},
            {make(constant, 0), make(constant, 1), make(mul), make(pop)},
        },
        ctc {
            "1 / 2",
            {{1}, {2}},
            {make(constant, 0), make(constant, 1), make(div), make(pop)},
        },
        ctc {
            "-1",
            {{1}},
            {make(constant, 0), make(minus), make(pop)},
        },
    };
    run(std::move(tests));
}

TEST(compiler, booleanExpressions)
{
    using enum opcodes;
    std::array tests {
        ctc {
            "true",
            {},
            {make(tru), make(pop)},
        },
        ctc {
            "false",
            {},
            {make(fals), make(pop)},
        },
        ctc {
            "1 > 2",
            {{1}, {2}},
            {make(constant, 0), make(constant, 1), make(greater_than), make(pop)},
        },
        ctc {
            "1 < 2",
            {{2}, {1}},
            {make(constant, 0), make(constant, 1), make(greater_than), make(pop)},
        },
        ctc {
            "1 == 2",
            {{1}, {2}},
            {make(constant, 0), make(constant, 1), make(equal), make(pop)},
        },
        ctc {
            "1 != 2",
            {{1}, {2}},
            {make(constant, 0), make(constant, 1), make(not_equal), make(pop)},
        },
        ctc {
            "true == false",
            {},
            {make(tru), make(fals), make(equal), make(pop)},
        },
        ctc {
            "true != false",
            {},
            {make(tru), make(fals), make(not_equal), make(pop)},
        },
        ctc {
            "!true",
            {},
            {make(tru), make(bang), make(pop)},
        },
    };
    run(std::move(tests));
}
TEST(compiler, conditionals)
{
    using enum opcodes;
    std::array tests {
        ctc {
            R"(if (true) { 10 }; 3333)",
            {{
                10,
                3333,
            }},
            {
                make(tru),
                make(jump_not_truthy, 10),
                make(constant, 0),
                make(jump, 11),
                make(null),
                make(pop),
                make(constant, 1),
                make(pop),
            },
        },
        ctc {
            R"(if (true) { 10 } else { 20 }; 3333)",
            {{
                10,
                20,
                3333,
            }},
            {
                make(tru),
                make(jump_not_truthy, 10),
                make(constant, 0),
                make(jump, 13),
                make(constant, 1),
                make(pop),
                make(constant, 2),
                make(pop),
            },
        },
    };
    run(std::move(tests));
}

TEST(compiler, globalLetStatements)
{
    using enum opcodes;
    std::array tests {
        ctc {
            R"(let one = 1;
               let two = 2;)",
            {{1, 2}},
            {
                make(constant, 0),
                make(set_global, 0),
                make(constant, 1),
                make(set_global, 1),
            },
        },
        ctc {
            R"(let one = 1;
               one;
        )",
            {{1}},
            {
                make(constant, 0),
                make(set_global, 0),
                make(get_global, 0),
                make(pop),
            },
        },
        ctc {
            R"(let one = 1;
               let two = one;
               two;
        )",
            {{1}},
            {
                make(constant, 0),
                make(set_global, 0),
                make(get_global, 0),
                make(set_global, 1),
                make(get_global, 1),
                make(pop),
            },
        },
    };

    run(std::move(tests));
}

TEST(compiler, stringExpression)
{
    using enum opcodes;
    std::array tests {
        ctc {
            R"("monkey")",
            {{"monkey"}},
            {
                make(constant, 0),
                make(pop),
            },
        },
        ctc {
            R"("mon" + "key")",
            {{"mon", "key"}},
            {
                make(constant, 0),
                make(constant, 1),
                make(add),
                make(pop),
            },
        },
    };
}

TEST(compiler, arrayLiterals)
{
    using enum opcodes;
    std::array tests {
        ctc {
            R"([])",
            {},
            {
                make(array, 0),
                make(pop),
            },
        },
        ctc {
            R"([1, 2, 3])",
            {{1, 2, 3}},
            {
                make(constant, 0),
                make(constant, 1),
                make(constant, 2),
                make(array, 3),
                make(pop),
            },
        },
        ctc {
            R"([1 + 2, 3 - 4, 5 * 6])",
            {{1, 2, 3, 4, 5, 6}},
            {
                make(constant, 0),
                make(constant, 1),
                make(add),
                make(constant, 2),
                make(constant, 3),
                make(sub),
                make(constant, 4),
                make(constant, 5),
                make(mul),
                make(array, 3),
                make(pop),
            },
        },
    };
    run(std::move(tests));
}

TEST(compiler, hashLiterals)
{
    using enum opcodes;
    std::array tests {
        ctc {
            R"({})",
            {},
            {
                make(hash, 0),
                make(pop),
            },
        },
        ctc {
            R"({1: 2, 3: 4, 5: 6 })",
            {{1, 2, 3, 4, 5, 6}},
            {
                make(constant, 0),
                make(constant, 1),
                make(constant, 2),
                make(constant, 3),
                make(constant, 4),
                make(constant, 5),
                make(hash, 6),
                make(pop),
            },
        },
        ctc {
            R"({1: 2 + 3, 4: 5 * 6})",
            {{1, 2, 3, 4, 5, 6}},
            {
                make(constant, 0),
                make(constant, 1),
                make(constant, 2),
                make(add),
                make(constant, 3),
                make(constant, 4),
                make(constant, 5),
                make(mul),
                make(hash, 4),
                make(pop),
            },
        },
    };
    run(std::move(tests));
}

TEST(compiler, indexExpression)
{
    using enum opcodes;
    std::array tests {
        ctc {
            R"([1, 2, 3][1 + 1])",
            {1, 2, 3, 1, 1},
            {
                make(constant, 0),
                make(constant, 1),
                make(constant, 2),
                make(array, 3),
                make(constant, 3),
                make(constant, 4),
                make(add),
                make(index),
                make(pop),
            },
        },
        ctc {
            R"({1: 2}[2 - 1])",
            {1, 2, 2, 1},
            {
                make(constant, 0),
                make(constant, 1),
                make(hash, 2),
                make(constant, 2),
                make(constant, 3),
                make(sub),
                make(index),
                make(pop),
            },
        },
    };
    run(std::move(tests));
}

TEST(compiler, functions)
{
    using enum opcodes;
    std::array tests {
        ctc {
            R"(fn() { return  5 + 10 })",
            {
                5,
                10,
                maker({make(constant, 0), make(constant, 1), make(add), make(return_value)}),
            },
            {
                make(closure, {2, 0}),
                make(pop),
            },
        },
        ctc {
            R"(fn() { 5 + 10 })",
            {
                5,
                10,
                maker({make(constant, 0), make(constant, 1), make(add), make(return_value)}),
            },
            {
                make(closure, {2, 0}),
                make(pop),
            },
        },
        ctc {
            R"(fn() { 1; 2})",
            {
                1,
                2,
                maker({make(constant, 0), make(pop), make(constant, 1), make(return_value)}),
            },
            {
                make(closure, {2, 0}),
                make(pop),
            },
        },
        ctc {
            R"(fn() { })",
            {
                maker({make(ret)}),
            },
            {
                make(closure, {0, 0}),
                make(pop),
            },
        },
    };
    run(std::move(tests));
}

TEST(compiler, functionCalls)
{
    using enum opcodes;
    std::array tests {
        ctc {
            R"(fn() { 24 }();)",
            {
                24,
                maker({make(constant, 0), make(return_value)}),
            },
            {
                make(closure, {1, 0}),
                make(call, 0),
                make(pop),
            },
        },
        ctc {
            R"(let noArg = fn() { 24 };
               noArg())",
            {
                24,
                maker({make(constant, 0), make(return_value)}),
            },
            {
                make(closure, {1, 0}),
                make(set_global, 0),
                make(get_global, 0),
                make(call, 0),
                make(pop),
            },
        },
        ctc {
            R"(let oneArg = fn(a) { };
               oneArg(24);)",
            {
                maker({make(ret)}),
                24,
            },
            {
                make(closure, {0, 0}),
                make(set_global, 0),
                make(get_global, 0),
                make(constant, 1),
                make(call, 1),
                make(pop),
            },
        },
        ctc {
            R"(let manyArg = fn(a, b, c) { };
               manyArg(24, 25, 26);)",
            {
                maker({make(ret)}),
                24,
                25,
                26,
            },
            {
                make(closure, {0, 0}),
                make(set_global, 0),
                make(get_global, 0),
                make(constant, 1),
                make(constant, 2),
                make(constant, 3),
                make(call, 3),
                make(pop),
            },
        },
        ctc {
            R"(let oneArg = fn(a) { a };
               oneArg(24);)",
            {
                maker({make(get_local, 0), make(return_value)}),
                24,
            },
            {
                make(closure, {0, 0}),
                make(set_global, 0),
                make(get_global, 0),
                make(constant, 1),
                make(call, 1),
                make(pop),
            },
        },
        ctc {
            R"(let manyArg = fn(a, b, c) { a; b; c; }
               manyArg(24, 25, 26);)",
            {
                maker({make(get_local, 0),
                       make(pop),
                       make(get_local, 1),
                       make(pop),
                       make(get_local, 2),
                       make(return_value)}),
                24,
                25,
                26,
            },
            {
                make(closure, {0, 0}),
                make(set_global, 0),
                make(get_global, 0),
                make(constant, 1),
                make(constant, 2),
                make(constant, 3),
                make(call, 3),
                make(pop),
            },
        },
    };
    run(std::move(tests));
}

TEST(compiler, letStatementScopes)
{
    using enum opcodes;
    std::array tests {
        ctc {
            R"(let num = 55; fn() { num };)",
            {
                55,
                maker({make(get_global, 0), make(return_value)}),
            },
            {
                make(constant, 0),
                make(set_global, 0),
                make(closure, {1, 0}),
                make(pop),
            },
        },
        ctc {
            R"(fn() {
                let num = 55;
                num 
            })",
            {
                55,
                maker({make(constant, 0), make(set_local, 0), make(get_local, 0), make(return_value)}),
            },
            {
                make(closure, {1, 0}),
                make(pop),
            },
        },
        ctc {
            R"(fn() {
                let a = 55;
                let b = 77;
                a + b 
            })",
            {
                55,
                77,
                maker({make(constant, 0),
                       make(set_local, 0),
                       make(constant, 1),
                       make(set_local, 1),
                       make(get_local, 0),
                       make(get_local, 1),
                       make(add),
                       make(return_value)}),
            },
            {
                make(closure, {2, 0}),
                make(pop),
            },
        },
    };
    run(std::move(tests));
}

TEST(compiler, builtins)
{
    using enum opcodes;
    std::array tests {
        ctc {
            R"(
            len([]);
            push([], 1);
            )",
            {1},
            {
                make(get_builtin, 0),
                make(array, 0),
                make(call, 1),
                make(pop),
                make(get_builtin, 5),
                make(array, 0),
                make(constant, 0),
                make(call, 2),
                make(pop),

            },
        },
        ctc {
            R"(
            fn() { len([]) }
            )",
            {maker({make(get_builtin, 0), make(array, 0), make(call, 1), make(return_value)})},
            {
                make(closure, {0, 0}),
                make(pop),
            },
        },
    };
    run(std::move(tests));
}
TEST(compiler, closures)
{
    using enum opcodes;
    std::array tests {
        ctc {
            R"(
            fn(a) {
                fn(b) {
                    a + b;
                }
            }
            )",
            {maker({make(get_free, 0), make(get_local, 0), make(add), make(return_value)}),
             maker({make(get_local, 0), make(closure, {0, 1}), make(return_value)})},
            {
                make(closure, {1, 0}),
                make(pop),
            },
        },
        ctc {
            R"(fn(a) {
                 fn(b) {
                    fn(c) {
                        a + b + c
                    }
                }
            };)",
            {maker(
                 {make(get_free, 0), make(get_free, 1), make(add), make(get_local, 0), make(add), make(return_value)}),
             maker({make(get_free, 0), make(get_local, 0), make(closure, {0, 2}), make(return_value)}),
             maker({make(get_local, 0), make(closure, {1, 1}), make(return_value)})},
            {
                make(closure, {2, 0}),
                make(pop),
            }},
        ctc {
            R"(
            let global = 55;
            fn() {
                let a = 66;

                fn() {
                    let b = 77;

                    fn() {
                        let c = 88;

                        global + a + b + c;
                    }
                }
            })",
            {55,
             66,
             77,
             88,
             maker({make(constant, 3),
                    make(set_local, 0),
                    make(get_global, 0),
                    make(get_free, 0),
                    make(add),
                    make(get_free, 1),
                    make(add),
                    make(get_local, 0),
                    make(add),
                    make(return_value)}),
             maker({make(constant, 2),
                    make(set_local, 0),
                    make(get_free, 0),
                    make(get_local, 0),
                    make(closure, {4, 2}),
                    make(return_value)}),
             maker({make(constant, 1),
                    make(set_local, 0),
                    make(get_local, 0),
                    make(closure, {5, 1}),
                    make(return_value)})},
            {
                make(constant, 0),
                make(set_global, 0),
                make(closure, {6, 0}),
                make(pop),
            }},
    };
    run(std::move(tests));
}
// NOLINTEND(*-magic-numbers)
