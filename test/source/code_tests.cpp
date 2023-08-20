#include <cstdint>

#include <compiler/code.hpp>
#include <gtest/gtest.h>

#include "testutils.hpp"

// NOLINTBEGIN(*-magic-numbers)
TEST(code, make)
{
    struct test
    {
        opcodes opcode;
        operands opers;
        instructions expected;
    };
    using enum opcodes;
    std::array tests {
        test {
            constant,
            {65534},
            {static_cast<uint8_t>(constant), 255, 254},
        },
        test {
            add,
            {},
            {static_cast<uint8_t>(add)},
        },
        test {
            pop,
            {},
            {static_cast<uint8_t>(pop)},
        },
        test {
            get_local,
            {255},
            {static_cast<uint8_t>(get_local), 255},
        },
        test {
            closure,
            {65534, 255},
            {static_cast<uint8_t>(closure), 255, 254, 255},
        },
    };
    for (auto&& [opcode, operands, expected] : tests) {
        auto actual = make(opcode, operands);
        ASSERT_EQ(actual, expected);
    };
}

TEST(code, instructionsToString)
{
    const auto* const expected = R"(0000 OpAdd
0001 OpGetLocal 1
0003 OpConstant 2
0006 OpConstant 65535
0009 OpClosure 65535 255
)";
    std::vector<instructions> instrs {
        make(opcodes::add),
        make(opcodes::get_local, 1),
        make(opcodes::constant, 2),
        make(opcodes::constant, 65535),
        make(opcodes::closure, {65535, 255}),
    };
    auto concatenated = flatten(instrs);
    auto actual = to_string(concatenated);
    ASSERT_EQ(expected, actual);
}

TEST(code, readOperands)
{
    struct test
    {
        opcodes opcode;
        operands opers;
        int bytes_read;
    };

    std::array tests {
        test {
            opcodes::constant,
            {65534},
            2,
        },
    };
    for (auto&& [opcode, operands, bytes] : tests) {
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
