#include <cstddef>
#include <iterator>

#include "compiler.hpp"

#include "compiler/code.hpp"
#include "symbol_table.hpp"

compiler::compiler(constants_ptr&& consts, symbol_table_ptr symbols)
    : consts {std::move(consts)}
    , scopes {1}
    , symbols {std::move(symbols)}
{
}

auto compiler::compile(const program_ptr& program) -> void
{
    program->compile(*this);
}

auto compiler::add_constant(object&& obj) -> size_t
{
    consts->push_back(std::move(obj));
    return consts->size() - 1;
}

auto compiler::add_instructions(instructions&& ins) -> size_t
{
    auto& scope = scopes[scope_index];
    auto pos = scope.instrs.size();
    std::copy(ins.begin(), ins.end(), std::back_inserter(scope.instrs));
    return pos;
}

auto compiler::emit(opcodes opcode, operands&& operands) -> size_t
{
    auto& scope = scopes[scope_index];
    scope.previous_instr = scope.last_instr;

    auto instr = make(opcode, std::move(operands));
    auto pos = add_instructions(std::move(instr));
    scope.last_instr.opcode = opcode;
    scope.last_instr.position = pos;
    return pos;
}

auto compiler::last_instruction_is(opcodes opcode) const -> bool
{
    if (current_instrs().empty()) {
        return false;
    }
    return scopes[scope_index].last_instr.opcode == opcode;
}

auto compiler::remove_last_pop() -> void
{
    auto& scope = scopes[scope_index];
    scope.instrs.resize(scope.last_instr.position);
    scope.last_instr = scope.previous_instr;
}

auto compiler::replace_last_pop_with_return() -> void
{
    auto last = scopes[scope_index].last_instr.position;
    using enum opcodes;
    replace_instruction(last, make(return_value));
    scopes[scope_index].last_instr.opcode = return_value;
}

auto compiler::replace_instruction(size_t pos, const instructions& instr) -> void
{
    auto& scope = scopes[scope_index];
    for (auto idx = 0UL; const auto& inst : instr) {
        scope.instrs[pos + idx] = inst;
        idx++;
    }
}

auto compiler::change_operand(size_t pos, size_t operand) -> void
{
    auto& scope = scopes[scope_index];
    auto opcode = static_cast<opcodes>(scope.instrs.at(pos));
    auto instr = make(opcode, operand);
    replace_instruction(pos, instr);
}

auto compiler::current_instrs() const -> const instructions&
{
    return scopes[scope_index].instrs;
}

auto compiler::byte_code() const -> bytecode
{
    return {scopes[scope_index].instrs, consts};
}

auto compiler::enter_scope() -> void
{
    scopes.resize(scopes.size() + 1);
    scope_index++;
    symbols = symbol_table::create_enclosed(symbols);
}

auto compiler::leave_scope() -> instructions
{
    auto instrs = scopes[scope_index].instrs;
    scopes.pop_back();
    scope_index--;
    symbols = symbols->parent();
    return instrs;
}
