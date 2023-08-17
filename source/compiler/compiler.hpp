#pragma once

#include "code.hpp"
#include "object.hpp"
#include "program.hpp"
#include "symbol_table.hpp"

using constants = std::vector<object>;
using constants_ptr = std::shared_ptr<constants>;

struct bytecode
{
    instructions instrs {};
    constants_ptr consts;
};

struct emitted_instruction
{
    opcodes opcode {};
    size_t position {};
};

struct compiler
{
    auto compile(const program_ptr& program) -> void;
    auto add_constant(object&& obj) -> size_t;
    auto add_instructions(instructions&& ins) -> size_t;
    auto emit(opcodes opcode, std::vector<int>&& operands = {}) -> size_t;
    auto last_is_pop() const -> bool;
    auto remove_last_pop() -> void;
    auto replace_instruction(size_t pos, const instructions& instr) -> void;
    auto change_operand(size_t pos, int operand) -> void;

    bytecode code;
    emitted_instruction last_instr {};
    emitted_instruction previous_instr {};
    symbol_table_ptr symbols;

    static inline auto create() -> compiler
    {
        return compiler {{.consts = std::make_shared<constants>()}, std::make_shared<symbol_table>()};
    }

    static inline auto create_with_state(constants_ptr constants, symbol_table_ptr symbols) -> compiler
    {
        return compiler {{.consts = std::move(constants)}, std::move(symbols)};
    }

  private:
    compiler(bytecode&& code, symbol_table_ptr symbols);
};
