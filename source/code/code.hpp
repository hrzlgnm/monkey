#pragma once
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
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
    floor_div,
    mod,
    bit_and,
    bit_or,
    bit_xor,
    bit_lsh,
    bit_rsh,
    logical_and,
    logical_or,
    pop,
    tru,
    fals,
    equal,
    not_equal,
    greater_than,
    minus,
    bang,
    jump_not_truthy,
    jump,
    null,
    get_global,
    set_global,
    array,
    hash,
    index,
    call,
    brake,
    cont,
    return_value,
    ret,
    get_local,
    set_local,
    get_free,
    set_free,
    get_outer,
    set_outer,
    get_builtin,
    closure,
    current_closure,
};

auto operator<<(std::ostream& ostream, opcodes opcode) -> std::ostream&;

template<>
struct fmt::formatter<opcodes> : ostream_formatter
{
};

using operands = std::vector<std::size_t>;
using instructions = std::vector<uint8_t>;

struct definition
{
    std::string_view name;
    operands operand_widths;
};

using definition_type = std::map<opcodes, definition>;

const definition_type definitions {
    {opcodes::constant, definition {.name = "OpConstant", .operand_widths = {2}}},
    {opcodes::add, definition {.name = "OpAdd"}},
    {opcodes::sub, definition {.name = "OpSub"}},
    {opcodes::mul, definition {.name = "OpMul"}},
    {opcodes::div, definition {.name = "OpDiv"}},
    {opcodes::mod, definition {.name = "OpMod"}},
    {opcodes::floor_div, definition {.name = "OpFloorDiv"}},
    {opcodes::bit_and, definition {.name = "OpBitAnd"}},
    {opcodes::bit_or, definition {.name = "OpBitOr"}},
    {opcodes::bit_xor, definition {.name = "OpBitXor"}},
    {opcodes::bit_lsh, definition {.name = "OpBitLsh"}},
    {opcodes::bit_rsh, definition {.name = "OpBitRsh"}},
    {opcodes::logical_and, definition {.name = "OpLogicalAnd"}},
    {opcodes::logical_or, definition {.name = "OpLogicalOr"}},
    {opcodes::pop, definition {.name = "OpPop"}},
    {opcodes::tru, definition {.name = "OpTrue"}},
    {opcodes::fals, definition {.name = "OpFalse"}},
    {opcodes::equal, definition {.name = "OpEqual"}},
    {opcodes::not_equal, definition {.name = "OpNotEquql"}},
    {opcodes::greater_than, definition {.name = "OpGreaterThan"}},
    {opcodes::minus, definition {.name = "OpMinus"}},
    {opcodes::bang, definition {.name = "OpBang"}},
    {opcodes::jump_not_truthy, definition {.name = "OpJumpNotTruthy", .operand_widths = {2}}},
    {opcodes::jump, definition {.name = "OpJump", .operand_widths = {2}}},
    {opcodes::null, definition {.name = "OpNull"}},
    {opcodes::get_global, definition {.name = "OpGetGlobal", .operand_widths = {2}}},
    {opcodes::set_global, definition {.name = "OpSetGlobal", .operand_widths = {2}}},
    {opcodes::array, definition {.name = "OpArray", .operand_widths = {2}}},
    {opcodes::hash, definition {.name = "OpHash", .operand_widths = {2}}},
    {opcodes::index, definition {.name = "OpIndex"}},
    {opcodes::call, definition {.name = "OpCall", .operand_widths = {1}}},
    {opcodes::brake, definition {.name = "OpBreak"}},
    {opcodes::cont, definition {.name = "OpContinue"}},
    {opcodes::return_value, definition {.name = "OpReturnValue"}},
    {opcodes::ret, definition {.name = "OpReturn"}},
    {opcodes::get_local, definition {.name = "OpGetLocal", .operand_widths = {1}}},
    {opcodes::set_local, definition {.name = "OpSetLocal", .operand_widths = {1}}},
    {opcodes::get_free, definition {.name = "OpGetFree", .operand_widths = {1}}},
    {opcodes::set_free, definition {.name = "OpSetFree", .operand_widths = {1}}},
    {opcodes::set_outer, definition {.name = "OpSetOuter", .operand_widths = {1, 1, 1}}},
    {opcodes::get_outer, definition {.name = "OpGetOuter", .operand_widths = {1, 1, 1}}},
    {opcodes::get_builtin, definition {.name = "OpGetBuiltin", .operand_widths = {1}}},
    {opcodes::closure, definition {.name = "OpClosure", .operand_widths = {2, 1}}},
    {opcodes::current_closure, definition {.name = "OpCurrentClosure"}},
};

[[nodiscard]] auto make(opcodes opcode, const operands& operands = {}) -> instructions;
[[nodiscard]] auto make(opcodes opcode, size_t operand) -> instructions;
[[nodiscard]] auto lookup(opcodes opcode) -> std::optional<definition>;
[[nodiscard]] auto read_operands(const definition& def, const instructions& instr)
    -> std::pair<operands, operands::size_type>;
[[nodiscard]] auto to_string(const instructions& code) -> std::string;
[[nodiscard]] auto read_uint16_big_endian(const instructions& bytes, size_t offset) -> uint16_t;
void write_uint16_big_endian(instructions& bytes, size_t offset, uint16_t value);
