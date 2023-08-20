#include <ast/array_expression.hpp>
#include <ast/binary_expression.hpp>
#include <ast/boolean.hpp>
#include <ast/call_expression.hpp>
#include <ast/function_expression.hpp>
#include <ast/hash_literal_expression.hpp>
#include <ast/if_expression.hpp>
#include <ast/index_expression.hpp>
#include <ast/integer_literal.hpp>
#include <ast/program.hpp>
#include <ast/string_literal.hpp>
#include <ast/unary_expression.hpp>

#include "code.hpp"
#include "compiler.hpp"
#include "symbol_table.hpp"

auto array_expression::compile(compiler& comp) const -> void
{
    for (const auto& element : elements) {
        element->compile(comp);
    }
    comp.emit(opcodes::array, elements.size());
}

auto binary_expression::compile(compiler& comp) const -> void
{
    if (op == token_type::less_than) {
        right->compile(comp);
        left->compile(comp);
        comp.emit(opcodes::greater_than);
        return;
    }
    left->compile(comp);
    right->compile(comp);
    switch (op) {
        case token_type::plus:
            comp.emit(opcodes::add);
            break;
        case token_type::minus:
            comp.emit(opcodes::sub);
            break;
        case token_type::asterisk:
            comp.emit(opcodes::mul);
            break;
        case token_type::slash:
            comp.emit(opcodes::div);
            break;
        case token_type::greater_than:
            comp.emit(opcodes::greater_than);
            break;
        case token_type::equals:
            comp.emit(opcodes::equal);
            break;
        case token_type::not_equals:
            comp.emit(opcodes::not_equal);
            break;
        default:
            throw std::runtime_error(fmt::format("unsupported operator {}", op));
    }
}

auto boolean::compile(compiler& comp) const -> void
{
    comp.emit(value ? opcodes::tru : opcodes::fals);
}

auto hash_literal_expression::compile(compiler& comp) const -> void
{
    for (const auto& [key, value] : pairs) {
        key->compile(comp);
        value->compile(comp);
    }
    comp.emit(opcodes::hash, pairs.size() * 2);
}

auto identifier::compile(compiler& comp) const -> void
{
    auto maybe_symbol = comp.symbols->resolve(value);
    if (!maybe_symbol.has_value()) {
        throw std::runtime_error(fmt::format("undefined variable {}", value));
    }
    auto symbol = maybe_symbol.value();
    comp.load_symbol(symbol);
}

auto if_expression::compile(compiler& comp) const -> void
{
    condition->compile(comp);
    using enum opcodes;
    auto jump_not_truthy_pos = comp.emit(jump_not_truthy, 0);
    consequence->compile(comp);
    if (comp.last_instruction_is(pop)) {
        comp.remove_last_pop();
    }
    auto jump_pos = comp.emit(jump, 0);
    auto after_consequence = comp.current_instrs().size();
    comp.change_operand(jump_not_truthy_pos, after_consequence);

    if (!alternative) {
        comp.emit(null);
    } else {
        alternative->compile(comp);
        if (comp.last_instruction_is(pop)) {
            comp.remove_last_pop();
        }
    }
    auto after_alternative = comp.current_instrs().size();
    comp.change_operand(jump_pos, after_alternative);
}

auto index_expression::compile(compiler& comp) const -> void
{
    left->compile(comp);
    index->compile(comp);
    comp.emit(opcodes::index);
}

auto integer_literal::compile(compiler& comp) const -> void
{
    comp.emit(opcodes::constant, comp.add_constant({value}));
}

auto program::compile(compiler& comp) const -> void
{
    for (const auto& stmt : statements) {
        stmt->compile(comp);
    }
}

auto let_statement::compile(compiler& comp) const -> void
{
    auto symbol = comp.symbols->define(name->value);
    value->compile(comp);

    if (symbol.is_local()) {
        comp.emit(opcodes::set_local, symbol.index);
    } else {
        comp.emit(opcodes::set_global, symbol.index);
    }
}

auto return_statement::compile(compiler& comp) const -> void
{
    value->compile(comp);
    comp.emit(opcodes::return_value);
}

auto expression_statement::compile(compiler& comp) const -> void
{
    expr->compile(comp);
    comp.emit(opcodes::pop);
}

auto block_statement::compile(compiler& comp) const -> void
{
    for (const auto& stmt : statements) {
        stmt->compile(comp);
    }
}

auto string_literal::compile(compiler& comp) const -> void
{
    comp.emit(opcodes::constant, comp.add_constant({value}));
}

auto unary_expression::compile(compiler& comp) const -> void
{
    right->compile(comp);
    switch (op) {
        case token_type::exclamation:
            comp.emit(opcodes::bang);
            break;
        case token_type::minus:
            comp.emit(opcodes::minus);
            break;
        default:
            throw std::runtime_error(fmt::format("invalid operator {}", op));
    }
}

auto function_expression::compile(compiler& comp) const -> void
{
    comp.enter_scope();
    if (!name.empty()) {
        comp.symbols->define_function_name(name);
    }
    for (const auto& param : parameters) {
        comp.symbols->define(param);
    }
    body->compile(comp);

    using enum opcodes;
    if (comp.last_instruction_is(pop)) {
        comp.replace_last_pop_with_return();
    }
    if (!comp.last_instruction_is(return_value)) {
        comp.emit(ret);
    }
    auto free = comp.symbols->free();
    auto num_locals = comp.symbols->num_definitions();
    auto instrs = comp.leave_scope();
    for (const auto& sym : free) {
        comp.load_symbol(sym);
    }
    auto function_index = comp.add_constant({compiled_function {instrs, num_locals, parameters.size()}});
    comp.emit(closure, {function_index, free.size()});
}

auto call_expression::compile(compiler& comp) const -> void
{
    function->compile(comp);
    for (const auto& arg : arguments) {
        arg->compile(comp);
    }
    comp.emit(opcodes::call, arguments.size());
}
