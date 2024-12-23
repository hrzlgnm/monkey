#include <stdexcept>
#include <utility>

#include <ast/array_expression.hpp>
#include <ast/binary_expression.hpp>
#include <ast/boolean.hpp>
#include <ast/builtin_function_expression.hpp>
#include <ast/call_expression.hpp>
#include <ast/decimal_literal.hpp>
#include <ast/expression.hpp>
#include <ast/function_expression.hpp>
#include <ast/hash_literal_expression.hpp>
#include <ast/identifier.hpp>
#include <ast/if_expression.hpp>
#include <ast/index_expression.hpp>
#include <ast/integer_literal.hpp>
#include <ast/program.hpp>
#include <ast/statements.hpp>
#include <ast/string_literal.hpp>
#include <ast/unary_expression.hpp>
#include <code/code.hpp>
#include <fmt/format.h>
#include <gc.hpp>
#include <lexer/token_type.hpp>
#include <object/object.hpp>

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
        case token_type::percent:
            comp.emit(opcodes::mod);
            break;
        case token_type::double_slash:
            comp.emit(opcodes::floor_div);
            break;
        case token_type::ampersand:
            comp.emit(opcodes::bit_and);
            break;
        case token_type::pipe:
            comp.emit(opcodes::bit_or);
            break;
        case token_type::caret:
            comp.emit(opcodes::bit_xor);
            break;
        case token_type::shift_left:
            comp.emit(opcodes::bit_lsh);
            break;
        case token_type::shift_right:
            comp.emit(opcodes::bit_rsh);
            break;
        case token_type::logical_and:
            comp.emit(opcodes::logical_and);
            break;
        case token_type::logical_or:
            comp.emit(opcodes::logical_or);
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
    auto maybe_symbol = comp.resolve_symbol(value);
    if (!maybe_symbol.has_value()) {
        throw std::runtime_error(fmt::format("undefined variable {}", value));
    }
    const auto& symbol = maybe_symbol.value();
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

    if (alternative == nullptr) {
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

auto while_statement::compile(compiler& comp) const -> void
{
    using enum opcodes;
    auto loop_start_pos = comp.current_instrs().size();
    condition->compile(comp);
    auto jump_not_truthy_pos = comp.emit(jump_not_truthy, 0);

    /* create a closure for the while loop body */
    comp.enter_scope(/*inside_loop=*/true);
    body->compile(comp);
    /* add a continue opcode at the end, to detect whether break was called or not */
    comp.emit(cont);

    auto free = comp.free_symbols();
    auto num_locals = comp.number_symbol_definitions();
    auto instrs = comp.leave_scope();
    for (const auto& sym : free) {
        comp.load_symbol(sym);
    }
    auto* cmpl = make<compiled_function_object>(std::move(instrs), num_locals, 0);
    cmpl->inside_loop = true;
    auto function_index = comp.add_constant(cmpl);
    comp.emit(closure, {function_index, free.size()});
    comp.emit(call, 0);

    auto jump_on_break_pos = comp.emit(jump_not_truthy, 0);
    comp.emit(jump, loop_start_pos);

    const auto after_body_pos = comp.current_instrs().size();
    comp.change_operand(jump_not_truthy_pos, after_body_pos);
    comp.change_operand(jump_on_break_pos, after_body_pos);

    comp.emit(null);
    comp.emit(pop);
}

auto index_expression::compile(compiler& comp) const -> void
{
    left->compile(comp);
    index->compile(comp);
    comp.emit(opcodes::index);
}

auto integer_literal::compile(compiler& comp) const -> void
{
    comp.emit(opcodes::constant, comp.add_constant(make<integer_object>(value)));
}

auto decimal_literal::compile(compiler& comp) const -> void
{
    comp.emit(opcodes::constant, comp.add_constant(make<decimal_object>(value)));
}

auto program::compile(compiler& comp) const -> void
{
    for (const auto& stmt : statements) {
        stmt->compile(comp);
    }
}

auto let_statement::compile(compiler& comp) const -> void
{
    value->compile(comp);
    if (reassign) {
        const auto sym = comp.resolve_symbol(name->value).value();
        if (sym.scope == symbol_scope::global) {
            comp.emit(opcodes::set_global, sym.index);
        } else if (sym.scope == symbol_scope::local) {
            comp.emit(opcodes::set_local, sym.index);
        } else if (sym.scope == symbol_scope::free) {
            comp.emit(opcodes::set_free, sym.index);
        } else {
            const auto& val = sym.ptr.value();
            comp.emit(opcodes::set_outer,
                      {static_cast<operands::value_type>(val.level),
                       static_cast<operands::value_type>(val.scope),
                       static_cast<operands::value_type>(val.index)});
        }
    } else {
        const auto sym = comp.define_symbol(name->value);
        if (sym.is_local()) {
            comp.emit(opcodes::set_local, sym.index);
        } else {
            comp.emit(opcodes::set_global, sym.index);
        }
    }
}

auto return_statement::compile(compiler& comp) const -> void
{
    value->compile(comp);
    comp.emit(opcodes::return_value);
}

auto break_statement::compile(compiler& comp) const -> void
{
    comp.emit(opcodes::brake);
}

auto continue_statement::compile(compiler& comp) const -> void
{
    comp.emit(opcodes::cont);
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
    comp.emit(opcodes::constant, comp.add_constant(make<string_object>(value)));
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
        comp.define_function_name(name);
    }
    for (const auto& param : parameters) {
        comp.define_symbol(param);
    }
    body->compile(comp);

    using enum opcodes;
    if (comp.last_instruction_is(pop)) {
        comp.replace_last_pop_with_return();
    }
    if (!comp.last_instruction_is(return_value)) {
        comp.emit(ret);
    }
    auto free = comp.free_symbols();
    auto num_locals = comp.number_symbol_definitions();
    auto instrs = comp.leave_scope();
    for (const auto& sym : free) {
        comp.load_symbol(sym);
    }
    auto function_index =
        comp.add_constant(make<compiled_function_object>(std::move(instrs), num_locals, parameters.size()));
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

auto builtin_function_expression::compile(compiler& /*comp*/) const -> void {}
