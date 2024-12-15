#pragma once

#include <cstddef>

#include <ast/program.hpp>
#include <code/code.hpp>
#include <eval/object.hpp>

#include "symbol_table.hpp"

using constants = std::vector<const object*>;

struct bytecode
{
    instructions instrs;
    const constants* consts {};
};

struct emitted_instruction
{
    opcodes opcode {};
    size_t position {};
};

struct compilation_scope
{
    instructions instrs;
    emitted_instruction last_instr;
    emitted_instruction previous_instr;
};

struct compiler
{
    auto compile(const program* program) -> void;
    [[nodiscard]] static auto create() -> compiler;

    [[nodiscard]] static auto create_with_state(constants* constants, symbol_table* symbols) -> compiler
    {
        return compiler {constants, symbols};
    }

    [[nodiscard]] auto add_constant(object* obj) -> size_t;
    [[nodiscard]] auto add_instructions(const instructions& ins) -> size_t;
    auto emit(opcodes opcode, const operands& operands = {}) -> size_t;

    auto emit(opcodes opcode, size_t operand) -> size_t { return emit(opcode, std::vector {operand}); }

    [[nodiscard]] auto last_instruction_is(opcodes opcode) const -> bool;
    auto remove_last_pop() -> void;
    auto replace_last_pop_with_return() -> void;
    auto replace_instruction(size_t pos, const instructions& instr) -> void;
    auto change_operand(size_t pos, size_t operand) -> void;
    [[nodiscard]] auto byte_code() const -> bytecode;
    [[nodiscard]] auto current_instrs() const -> const instructions&;
    auto enter_scope() -> void;
    auto leave_scope() -> instructions;
    auto define_symbol(const std::string& name) -> symbol;
    auto define_function_name(const std::string& name) -> symbol;
    auto load_symbol(const symbol& sym) -> void;
    [[nodiscard]] auto resolve_symbol(const std::string& name) const -> std::optional<symbol>;
    [[nodiscard]] auto free_symbols() const -> std::vector<symbol>;
    [[nodiscard]] auto number_symbol_definitions() const -> size_t;
    [[nodiscard]] auto consts() const -> constants*;

  private:
    constants* m_consts {};
    symbol_table* m_symbols;
    std::vector<compilation_scope> m_scopes;
    size_t m_scope_index {0};
    compiler(constants* consts, symbol_table* symbols);
};
