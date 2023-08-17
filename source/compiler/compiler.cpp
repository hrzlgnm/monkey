#include <cstddef>
#include <iterator>

#include "compiler.hpp"

#include "compiler/code.hpp"

auto compiler::compile(const program_ptr& program) -> void
{
    program->compile(*this);
}

auto compiler::add_constant(object&& obj) -> size_t
{
    code.consts->push_back(std::move(obj));
    return code.consts->size() - 1;
}

auto compiler::add_instructions(instructions&& ins) -> size_t
{
    auto pos = code.code.size();
    std::copy(ins.begin(), ins.end(), std::back_inserter(code.code));
    return pos;
}

auto compiler::emit(opcodes opcode, std::vector<int>&& operands) -> size_t
{
    previous_instr = last_instr;

    auto instr = make(opcode, operands);
    auto pos = add_instructions(std::move(instr));
    last_instr.opcode = opcode;
    last_instr.position = pos;
    return pos;
}

auto compiler::last_is_pop() const -> bool
{
    return last_instr.opcode == opcodes::pop;
}

auto compiler::remove_last_pop() -> void
{
    code.code.resize(last_instr.position);
    last_instr = previous_instr;
}

auto compiler::replace_instruction(size_t pos, const instructions& instr) -> void
{
    for (auto idx = 0UL; const auto& inst : instr) {
        code.code[pos + idx] = inst;
        idx++;
    }
}

auto compiler::change_operand(size_t pos, int operand) -> void
{
    auto opcode = static_cast<opcodes>(code.code.at(pos));
    auto instr = make(opcode, operand);
    replace_instruction(pos, instr);
}
