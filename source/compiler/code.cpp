#include <cstdint>
#include <string>

#include "code.hpp"

#include <fmt/core.h>

#include "util.hpp"

auto make(opcodes opcode, const std::vector<int>& operands) -> instructions
{
    if (!definitions.contains(opcode)) {
        return {};
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
        }
        idx++;
    }
    return instr;
}

auto read_operands(const definition& def, const instructions& instr) -> std::pair<std::vector<int>, int>
{
    std::pair<std::vector<int>, int> result;
    result.first.resize(def.operand_widths.size());
    auto offset = 0;
    for (size_t idx = 0; const auto width : def.operand_widths) {
        switch (width) {
            case 2:
                result.first[idx] = read_uint16_big_endian(instr, offset);
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

auto fmt_instruction(const definition& def, const std::vector<int>& operands) -> std::string
{
    auto count = def.operand_widths.size();
    if (count != operands.size()) {
        return fmt::format("ERROR: operand len {} does not match defined {}\n", operands.size(), count);
    }
    switch (count) {
        case 1:
            return fmt::format("{} {}", def.name, operands.at(0));
    }
    return fmt::format("ERROR: unhandled operand count for {}", def.name);
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
        auto operand = read_operands(def.value(), {code.begin() + idx + 1, code.end()});
        result += fmt::format("{:04d} {}\n", idx, fmt_instruction(def.value(), operand.first));
        read = operand.second;
    }
    return result;
}
