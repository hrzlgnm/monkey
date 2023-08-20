#include <cstddef>
#include <iterator>

#include "compiler.hpp"

#include <ast/builtin_function_expression.hpp>

#include "code.hpp"
#include "symbol_table.hpp"

auto compiler::create() -> compiler
{
    auto symbols = symbol_table::create();
    for (size_t idx = 0; const auto& builtin : builtin_function_expression::builtins) {
        symbols->define_builtin(idx++, builtin.name);
    }
    return {std::make_shared<constants>(), std::move(symbols)};
}

compiler::compiler(constants_ptr&& consts, symbol_table_ptr symbols)
    : m_consts {std::move(consts)}
    , m_symbols {std::move(symbols)}
    , m_scopes {1}
{
}

auto compiler::compile(const program_ptr& program) -> void
{
    program->compile(*this);
}

auto compiler::add_constant(object&& obj) -> size_t
{
    m_consts->push_back(std::move(obj));
    return m_consts->size() - 1;
}

auto compiler::add_instructions(instructions&& ins) -> size_t
{
    auto& scope = m_scopes[m_scope_index];
    auto pos = scope.instrs.size();
    std::copy(ins.begin(), ins.end(), std::back_inserter(scope.instrs));
    return pos;
}

auto compiler::emit(opcodes opcode, operands&& operands) -> size_t
{
    auto& scope = m_scopes[m_scope_index];
    scope.previous_instr = scope.last_instr;

    auto instr = make(opcode, operands);
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
    return m_scopes[m_scope_index].last_instr.opcode == opcode;
}

auto compiler::remove_last_pop() -> void
{
    auto& scope = m_scopes[m_scope_index];
    scope.instrs.resize(scope.last_instr.position);
    scope.last_instr = scope.previous_instr;
}

auto compiler::replace_last_pop_with_return() -> void
{
    auto last = m_scopes[m_scope_index].last_instr.position;
    using enum opcodes;
    replace_instruction(last, make(return_value));
    m_scopes[m_scope_index].last_instr.opcode = return_value;
}

auto compiler::replace_instruction(size_t pos, const instructions& instr) -> void
{
    auto& scope = m_scopes[m_scope_index];
    for (auto idx = 0UL; const auto& inst : instr) {
        scope.instrs[pos + idx] = inst;
        idx++;
    }
}

auto compiler::change_operand(size_t pos, size_t operand) -> void
{
    auto& scope = m_scopes[m_scope_index];
    auto opcode = static_cast<opcodes>(scope.instrs.at(pos));
    auto instr = make(opcode, operand);
    replace_instruction(pos, instr);
}

auto compiler::current_instrs() const -> const instructions&
{
    return m_scopes[m_scope_index].instrs;
}

auto compiler::byte_code() const -> bytecode
{
    return {m_scopes[m_scope_index].instrs, m_consts};
}

auto compiler::enter_scope() -> void
{
    m_scopes.resize(m_scopes.size() + 1);
    m_scope_index++;
    m_symbols = symbol_table::create_enclosed(m_symbols);
}

auto compiler::leave_scope() -> instructions
{
    auto instrs = m_scopes[m_scope_index].instrs;
    m_scopes.pop_back();
    m_scope_index--;
    m_symbols = m_symbols->parent();
    return instrs;
}

auto compiler::define_symbol(const std::string& name) -> symbol
{
    return m_symbols->define(name);
}

auto compiler::define_function_name(const std::string& name) -> symbol
{
    return m_symbols->define_function_name(name);
}

auto compiler::load_symbol(const symbol& sym) -> void
{
    using enum symbol_scope;
    using enum opcodes;
    switch (sym.scope) {
        case global:
            emit(get_global, sym.index);
            break;
        case local:
            emit(get_local, sym.index);
            break;
        case builtin:
            emit(get_builtin, sym.index);
            break;
        case free:
            emit(get_free, sym.index);
            break;
        case function:
            emit(current_closure);
            break;
    }
}

auto compiler::resolve_symbol(const std::string& name) const -> std::optional<symbol>
{
    return m_symbols->resolve(name);
}

auto compiler::free_symbols() const -> std::vector<symbol>
{
    return m_symbols->free();
}

auto compiler::number_symbol_definitions() const -> size_t
{
    return m_symbols->num_definitions();
}

auto compiler::consts() const -> constants_ptr
{
    return m_consts;
}
