#include <cstdint>
#include <iterator>

#include "compiler/compiler.hpp"

#include <gtest/gtest.h>

#include "compiler/code.hpp"
#include "parser.hpp"
#include "testutils.hpp"

// NOLINTBEGIN(*-magic-numbers)
TEST(compiler, make)
{
    struct test
    {
        opcodes opcode;
        std::vector<int> operands;
        instructions expected;
    };
    std::array tests {
        test {
            opcodes::constant,
            {65534},
            {static_cast<uint8_t>(opcodes::constant), 255, 254},
        },
        test {
            opcodes::add,
            {},
            {static_cast<uint8_t>(opcodes::add)},
        },
        test {
            opcodes::pop,
            {},
            {static_cast<uint8_t>(opcodes::pop)},
        },
    };
    for (const auto& [opcode, operands, expected] : tests) {
        auto actual = make(opcode, operands);
        ASSERT_EQ(actual, expected);
    };
}

using expected_value = std::variant<int64_t>;

struct compiler_test_case
{
    std::string_view input;
    std::vector<expected_value> expected_constants;
    std::vector<instructions> expected_instructions;
};

template<typename T>
auto flatten(const std::vector<std::vector<T>>& arrs) -> std::vector<T>
{
    std::vector<T> result;
    for (const auto& arr : arrs) {
        std::copy(arr.cbegin(), arr.cend(), std::back_inserter(result));
    }
    return result;
}

auto assert_instructions(const std::vector<instructions>& instructions, const ::instructions& code)
{
    auto flattened = flatten(instructions);
    EXPECT_EQ(flattened.size(), code.size());
    EXPECT_EQ(flattened, code) << "expected " << to_string(flattened) << ", got " << to_string(code);
}

auto assert_constants(const std::vector<expected_value>& expecteds, const constants& consts)
{
    EXPECT_EQ(expecteds.size(), consts.size());
    for (size_t idx = 0; const auto& expected : expecteds) {
        const auto& actual = consts.at(idx);
        std::visit(
            overloaded {
                [&actual](const int64_t val) { ASSERT_EQ(val, actual.as<integer_value>()); },
            },
            expected);
        idx++;
    }
}

template<size_t N>
auto run_compiler_tests(std::array<compiler_test_case, N>&& tests)
{
    for (const auto& [input, constants, instructions] : tests) {
        auto [prgrm, _] = assert_program(input);
        compiler piler;
        piler.compile(prgrm);
        const auto bytecod = piler.code();
        assert_instructions(instructions, bytecod.code);
        assert_constants(constants, bytecod.consts);
    }
}

TEST(compiler, integerArithmetics)
{
    using enum opcodes;
    std::array tests {
        compiler_test_case {
            "1 + 2",
            {{1}, {2}},
            {make(constant, {0}), make(constant, {1}), make(add), make(pop)},
        },
        compiler_test_case {
            "1; 2",
            {{1}, {2}},
            {make(constant, {0}), make(pop), make(constant, {1}), make(pop)},
        },
        compiler_test_case {
            "1 - 2",
            {{1}, {2}},
            {make(constant, {0}), make(constant, {1}), make(sub), make(pop)},
        },
        compiler_test_case {
            "1 * 2",
            {{1}, {2}},
            {make(constant, {0}), make(constant, {1}), make(mul), make(pop)},
        },
        compiler_test_case {
            "1 / 2",
            {{1}, {2}},
            {make(constant, {0}), make(constant, {1}), make(div), make(pop)},
        },
    };
    run_compiler_tests(std::move(tests));
}
TEST(compiler, booleanExpressions)
{
    using enum opcodes;
    std::array tests {
        compiler_test_case {
            "true",
            {},
            {make(tru), make(pop)},
        },
        compiler_test_case {
            "false",
            {},
            {make(fals), make(pop)},
        },
    };
    run_compiler_tests(std::move(tests));
}

TEST(compiler, instructionsToString)
{
    const auto* const expected = R"(0000 OpAdd
0001 OpConstant 2
0004 OpConstant 65535
)";
    std::vector<instructions> instrs {
        make(opcodes::add, {}), make(opcodes::constant, {2}), make(opcodes::constant, {65535})};
    auto concatenated = flatten(instrs);
    auto actual = to_string(concatenated);
    ASSERT_EQ(expected, actual);
}

TEST(compiler, readOperands)
{
    struct test
    {
        opcodes opcode;
        std::vector<int> operands;
        int bytes_read;
    };
    std::array tests {
        test {
            opcodes::constant,
            {65534},
            2,
        },
    };
    for (const auto& [opcode, operands, bytes] : tests) {
        const auto instr = make(opcode, operands);
        const auto def = lookup(opcode);

        ASSERT_TRUE(def.has_value());

        const auto actual = read_operands(def.value(), {instr.begin() + 1, instr.end()});
        EXPECT_EQ(actual.second, bytes);
        for (size_t iidx = 0; iidx < operands.size(); ++iidx) {
            EXPECT_EQ(operands[iidx], actual.first[iidx]) << "at " << iidx;
        }
    }
}
// NOLINTEND(*-magic-numbers)
