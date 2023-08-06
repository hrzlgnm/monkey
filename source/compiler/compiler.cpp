#include <iterator>

#include "compiler.hpp"

#include "compiler/code.hpp"

auto compiler::compile(const program_ptr& program) -> void
{
    program->compile(*this);
}

auto compiler::code() const -> bytecode
{
    return {cod, consts};
}

auto compiler::add_constant(object&& obj) -> int
{
    consts.push_back(std::move(obj));
    return static_cast<int>(consts.size() - 1);
}

auto compiler::add_instructions(instructions&& ins) -> int
{
    auto pos = static_cast<int>(cod.size());
    std::copy(ins.begin(), ins.end(), std::back_inserter(cod));
    return pos;
}

auto compiler::emit(opcodes opcode, std::vector<int>&& operands) -> int
{
    auto instr = make(opcode, operands);
    auto pos = add_instructions(std::move(instr));
    return pos;
}
