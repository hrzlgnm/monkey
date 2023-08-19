#include <cstdint>
#include <stdexcept>
#include <string>

#include "code.hpp"

#include <fmt/core.h>
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
    }
    throw std::runtime_error(
        fmt::format("operator <<(std::ostream&) for {} is not implemented yet", static_cast<uint8_t>(opcode)));
}

auto make(opcodes opcode, operands&& operands) -> instructions
{
    if (!definitions.contains(opcode)) {
        throw std::invalid_argument(fmt::format("definition given opcode {} is not defined", opcode));
    }
    const auto& definition = definitions.at(opcode);
    instructions instr;
    instr.push_back(static_cast<uint8_t>(opcode));
    for (size_t idx = 0; const auto operand : operands) {
        auto width = definition.operand_widths.at(idx);
        switch (width) {
            case 2:
                write_uint16_big_endian(instr, instr.size(), static_cast<uint16_t>(operand));
                break;
            case 1:
                instr.push_back(static_cast<uint8_t>(operand));
                break;
        }
        break;
        idx++;
    }
    return instr;
}

auto make(opcodes opcode, size_t operand) -> instructions
{
    operands rands;
    rands.push_back(operand);
    return make(opcode, std::move(rands));
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
                result.first[idx] = instr.at(offset);
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
        return {};
    }
    return definitions.at(opcode);
}

auto fmt_instruction(const definition& def, const operands& operands) -> std::string
{
    auto count = def.operand_widths.size();
    if (count != operands.size()) {
        return fmt::format("ERROR: operand len {} does not match defined {}\n", operands.size(), count);
    }
    switch (count) {
        case 0:
            return def.name;
        case 1:
            return fmt::format("{} {}", def.name, operands.at(0));
        default:
            return fmt::format("ERROR: unhandled operand count for {}", def.name);
    }
}

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
