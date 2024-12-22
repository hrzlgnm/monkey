#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "code.hpp"

#include <doctest/doctest.h>
#include <fmt/format.h>

auto operator<<(std::ostream& ostream, opcodes opcode) -> std::ostream&
{
    using enum opcodes;
    switch (opcode) {
        case constant:
            return ostream << "constant";
        case add:
            return ostream << "add";
        case sub:
            return ostream << "sub";
        case mul:
            return ostream << "mul";
        case div:
            return ostream << "div";
        case opcodes::floor_div:
            return ostream << "floor_div";
        case pop:
            return ostream << "pop";
        case tru:
            return ostream << "tru";
        case fals:
            return ostream << "fals";
        case equal:
            return ostream << "equal";
        case not_equal:
            return ostream << "not_equal";
        case greater_than:
            return ostream << "greater_than";
        case minus:
            return ostream << "minus";
        case bang:
            return ostream << "bang";
        case jump_not_truthy:
            return ostream << "jump_not_truthy";
        case jump:
            return ostream << "jump";
        case null:
            return ostream << "null";
        case get_global:
            return ostream << "get_global";
        case set_global:
            return ostream << "set_global";
        case array:
            return ostream << "array";
        case hash:
            return ostream << "hash";
        case index:
            return ostream << "index";
        case call:
            return ostream << "call";
        case return_value:
            return ostream << "return_value";
        case ret:
            return ostream << "return";
        case get_local:
            return ostream << "get_local";
        case set_local:
            return ostream << "set_local";
        case get_builtin:
            return ostream << "get_builtin";
        case closure:
            return ostream << "closure";
        case get_free:
            return ostream << "get_free";
        case current_closure:
            return ostream << "current_closure";
        case opcodes::mod:
            return ostream << "mod";
        case opcodes::bit_and:
            return ostream << "bit_and";
        case opcodes::bit_or:
            return ostream << "bit_or";
        case opcodes::bit_xor:
            return ostream << "bit_xor";
        case opcodes::bit_lsh:
            return ostream << "bit_lsh";
        case opcodes::bit_rsh:
            return ostream << "bit_rsh";
        case opcodes::logical_and:
            return ostream << "logical_and";
        case opcodes::logical_or:
            return ostream << "logical_or";
    }
    throw std::runtime_error(
        fmt::format("operator <<(std::ostream&) for {} is not implemented yet", static_cast<uint8_t>(opcode)));
}

auto make(opcodes opcode, const operands& operands) -> instructions
{
    if (!definitions.contains(opcode)) {
        throw std::invalid_argument(fmt::format("definition given opcode {} is not defined", opcode));
    }
    const auto& definition = definitions.at(opcode);
    instructions instr;
    instr.push_back(static_cast<uint8_t>(opcode));
    for (size_t idx = 0; const auto operand : operands) {
        auto width = definition.operand_widths[idx];
        switch (width) {
            case 2:
                write_uint16_big_endian(
                    instr, static_cast<std::uint16_t>(instr.size()), static_cast<std::uint16_t>(operand));
                break;
            case 1:
                instr.push_back(static_cast<uint8_t>(operand));
                break;
            default:
                throw std::runtime_error(fmt::format("invalid operand width: {}", width));
        }
        idx++;
    }
    return instr;
}

auto make(opcodes opcode, size_t operand) -> instructions
{
    operands rands;
    rands.push_back(operand);
    return make(opcode, rands);
}

auto read_operands(const definition& def, const instructions& instr) -> std::pair<operands, operands::size_type>
{
    std::pair<operands, operands::size_type> result;
    result.first.resize(def.operand_widths.size());
    auto offset = 0UL;
    for (size_t idx = 0; const auto width : def.operand_widths) {
        switch (width) {
            case 2:
                result.first[idx] = read_uint16_big_endian(instr, offset);
                break;
            case 1:
                result.first[idx] = instr[offset];
                break;
            default:
                throw std::runtime_error(fmt::format("invalid operand width: {}", width));
        }
        offset += width;
        idx++;
    }
    result.second = offset;
    return result;
}

auto lookup(opcodes opcode) -> std::optional<definition>
{
    if (!definitions.contains(opcode)) {
        return std::nullopt;
    }
    return definitions.at(opcode);
}

namespace
{
auto fmt_instruction(const definition& def, const operands& operands) -> std::string
{
    auto count = def.operand_widths.size();
    if (count != operands.size()) {
        return fmt::format("ERROR: operand len {} does not match defined {}\n", operands.size(), count);
    }
    switch (count) {
        case 0:
            return std::string(def.name);
        case 1:
            return fmt::format("{} {}", def.name, operands[0]);
        case 2:
            return fmt::format("{} {} {}", def.name, operands[0], operands[1]);
        default:
            return fmt::format("ERROR: unhandled operand count for {}", def.name);
    }
}

}  // namespace

auto to_string(const instructions& code) -> std::string
{
    std::string result;
    auto read = 0U;
    for (size_t idx = 0; idx < code.size(); idx += 1 + read) {
        auto def = lookup(static_cast<opcodes>(code[idx]));
        if (!def.has_value()) {
            continue;
        }
        auto operand = read_operands(def.value(), {code.begin() + static_cast<int64_t>(idx) + 1, code.end()});
        result += fmt::format("{:04d} {}\n", idx, fmt_instruction(def.value(), operand.first));
        read = static_cast<unsigned>(operand.second);
    }
    return result;
}

constexpr auto bits_in_byte = 8U;
constexpr auto byte_mask = 0xFFU;

auto read_uint16_big_endian(const std::vector<uint8_t>& bytes, size_t offset) -> uint16_t
{
    if (offset + 2 > bytes.size()) {
        throw std::out_of_range("Offset is out of bounds");
    }

    auto result = static_cast<uint16_t>((bytes[offset]) << bits_in_byte);
    result |= bytes[offset + 1];

    return result;
}

void write_uint16_big_endian(std::vector<uint8_t>& bytes, size_t offset, uint16_t value)
{
    if (offset + 2 > bytes.size()) {
        bytes.resize(offset + 2);
    }

    bytes[offset] = static_cast<uint8_t>(value >> bits_in_byte);
    bytes[offset + 1] = static_cast<uint8_t>(value & byte_mask);
}

namespace
{
// NOLINTBEGIN(*)

template<typename T>
auto flatten(const std::vector<std::vector<T>>& arrs) -> std::vector<T>
{
    std::vector<T> result;
    for (const auto& arr : arrs) {
        std::copy(arr.cbegin(), arr.cend(), std::back_inserter(result));
    }
    return result;
}

TEST_SUITE("code")
{
    TEST_CASE("make")
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
            REQUIRE_EQ(actual, expected);
        }
    }

    TEST_CASE("instructionsToString")
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
        REQUIRE_EQ(expected, actual);
    }

    TEST_CASE("readOperands")
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

            REQUIRE(def.has_value());

            const auto actual = read_operands(def.value(), {instr.begin() + 1, instr.end()});
            CHECK_EQ(actual.second, bytes);
            for (size_t iidx = 0; iidx < operands.size(); ++iidx) {
                INFO("at", iidx);
                CHECK_EQ(operands[iidx], actual.first[iidx]);
            }
        }
    }
}

// NOLINTEND(*)
}  // namespace
