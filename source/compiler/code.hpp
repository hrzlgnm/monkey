#pragma once
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <fmt/ostream.h>

using instructions = std::vector<uint8_t>;

enum class opcodes : uint8_t
{
    constant,
    add,
    sub,
    mul,
    div,
    pop,
    tru,
    fals,
    equal,
    not_equal,
    greater_than,
    minus,
    bang,
};

auto operator<<(std::ostream& ostream, opcodes opcode) -> std::ostream&;

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
    {opcodes::tru, definition {"OpTrue"}},
    {opcodes::fals, definition {"OpFalse"}},
    {opcodes::equal, definition {"OpEqual"}},
    {opcodes::not_equal, definition {"OpNotEquql"}},
    {opcodes::greater_than, definition {"OpGreaterThan"}},
    {opcodes::minus, definition {"OpMinus"}},
    {opcodes::bang, definition {"OpBang"}},

};

auto make(opcodes opcode, const std::vector<int>& operands = {}) -> instructions;
auto lookup(opcodes opcode) -> std::optional<definition>;
auto read_operands(const definition& def, const instructions& instr) -> std::pair<std::vector<int>, int>;
auto to_string(const instructions& code) -> std::string;
