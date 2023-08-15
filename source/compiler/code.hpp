#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

using instructions = std::vector<uint8_t>;
using opcode = uint8_t;

enum class opcodes : uint8_t
{
    constant,
    add,
    sub,
    mul,
    div,
    pop
};

struct definition
{
    std::string name;
    std::vector<int> operand_widths {};
};

using definition_type = std::map<opcodes, definition>;

const definition_type definitions {
    {opcodes::constant, definition {"OpConstant", {2}}},
    {opcodes::add, definition {"OpAdd"}},
    {opcodes::sub, definition {"OpSub"}},
    {opcodes::mul, definition {"OpMul"}},
    {opcodes::div, definition {"OpDiv"}},
    {opcodes::pop, definition {"OpPop"}},
};

auto make(opcodes opcode, const std::vector<int>& operands = {}) -> instructions;
auto lookup(opcodes opcode) -> std::optional<definition>;
auto read_operands(const definition& def, const instructions& instr) -> std::pair<std::vector<int>, int>;
auto to_string(const instructions& code) -> std::string;
