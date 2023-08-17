#pragma once

#include "code.hpp"
#include "object.hpp"
#include "program.hpp"

using constants = std::vector<object>;

struct bytecode
{
    instructions code;
    constants consts;
};

struct emitted_instruction
{
    opcodes opcode {};
    size_t position {};
};

struct compiler
{
    instructions cod;
    constants consts;
    emitted_instruction last_instr {};
    emitted_instruction previous_instr {};
    auto compile(const program_ptr& program) -> void;
    auto code() const -> bytecode;
    auto add_constant(object&& obj) -> size_t;
    auto add_instructions(instructions&& ins) -> size_t;
    auto emit(opcodes opcode, std::vector<int>&& operands = {}) -> size_t;
    auto last_is_pop() const -> bool;
    auto remove_last_pop() -> void;
    auto replace_instruction(size_t pos, const instructions& instr) -> void;
    auto change_operand(size_t pos, int operand) -> void;
};
