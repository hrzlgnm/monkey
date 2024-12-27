#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "compiler.hpp"

#include <ast/array_literal.hpp>
#include <ast/assign_expression.hpp>
#include <ast/binary_expression.hpp>
#include <ast/boolean_literal.hpp>
#include <ast/call_expression.hpp>
#include <ast/decimal_literal.hpp>
#include <ast/expression.hpp>
#include <ast/function_literal.hpp>
#include <ast/hash_literal.hpp>
#include <ast/identifier.hpp>
#include <ast/if_expression.hpp>
#include <ast/index_expression.hpp>
#include <ast/integer_literal.hpp>
#include <ast/program.hpp>
#include <ast/statements.hpp>
#include <ast/string_literal.hpp>
#include <ast/unary_expression.hpp>
#include <builtin/builtin.hpp>
#include <code/code.hpp>
#include <doctest/doctest.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <gc.hpp>
#include <lexer/token_type.hpp>
#include <object/object.hpp>
#include <overloaded.hpp>
#include <parser/parser.hpp>

#include "symbol_table.hpp"

auto compiler::create() -> compiler
{
    auto* symbols = symbol_table::create();
    for (auto idx = 0; const auto& builtin : builtin::builtins()) {
        symbols->define_builtin(idx++, builtin->name);
    }
    return {make<constants>(), symbols};
}

compiler::compiler(constants* consts, symbol_table* symbols)
    : m_consts {consts}
    , m_symbols {symbols}
    , m_scopes {1}
{
}

auto compiler::compile(const program* program) -> void
{
    program->accept(*this);
}

auto compiler::add_constant(object* obj) -> std::size_t
{
    m_consts->push_back(obj);
    return m_consts->size() - 1;
}

auto compiler::add_instructions(const instructions& ins) -> std::size_t
{
    auto& scope = m_scopes[m_scope_index];
    auto pos = scope.instrs.size();
    std::copy(ins.cbegin(), ins.cend(), std::back_inserter(scope.instrs));
    return pos;
}

auto compiler::emit(opcodes opcode, const operands& operands) -> std::size_t
{
    auto& scope = m_scopes[m_scope_index];
    scope.previous_instr = scope.last_instr;

    auto instr = make(opcode, operands);
    auto pos = add_instructions(instr);
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

auto compiler::replace_instruction(std::size_t pos, const instructions& instr) -> void
{
    auto& scope = m_scopes[m_scope_index];
    for (auto idx = 0UL; const auto& inst : instr) {
        scope.instrs[pos + idx] = inst;
        idx++;
    }
}

auto compiler::change_operand(std::size_t pos, std::size_t operand) -> void
{
    auto& scope = m_scopes[m_scope_index];
    auto opcode = static_cast<opcodes>(scope.instrs[pos]);
    auto instr = make(opcode, operand);
    replace_instruction(pos, instr);
}

auto compiler::current_instrs() const -> const instructions&
{
    return m_scopes[m_scope_index].instrs;
}

auto compiler::byte_code() const -> bytecode
{
    return {.instrs = m_scopes[m_scope_index].instrs, .consts = m_consts};
}

auto compiler::enter_scope(bool inside_loop) -> void
{
    m_scopes.resize(m_scopes.size() + 1);
    m_scope_index++;
    m_symbols = symbol_table::create_enclosed(m_symbols, inside_loop);
}

auto compiler::leave_scope() -> instructions
{
    auto instrs = m_scopes[m_scope_index].instrs;
    m_scopes.pop_back();
    m_scope_index--;
    m_symbols = m_symbols->outer();
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
        case outer: {
            assert(sym.ptr.has_value());
            const auto& val = sym.ptr.value();
            emit(get_outer,
                 {
                     static_cast<operands::value_type>(val.level),
                     static_cast<operands::value_type>(val.scope),
                     static_cast<operands::value_type>(val.index),
                 });
        } break;
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

auto compiler::number_symbol_definitions() const -> int
{
    return m_symbols->num_definitions();
}

auto compiler::consts() const -> constants*
{
    return m_consts;
}

void compiler::visit(const array_literal& expr)
{
    for (const auto& element : expr.elements) {
        element->accept(*this);
    }
    emit(opcodes::array, expr.elements.size());
}

void compiler::visit(const assign_expression& expr)
{
    expr.value->accept(*this);
    const auto maybe_symbol = resolve_symbol(expr.name->value);
    assert(maybe_symbol.has_value());
    const auto& sym = maybe_symbol.value();
    if (sym.scope == symbol_scope::global) {
        emit(opcodes::set_global, sym.index);
    } else if (sym.scope == symbol_scope::local) {
        emit(opcodes::set_local, sym.index);
    } else if (sym.scope == symbol_scope::free) {
        emit(opcodes::set_free, sym.index);
    } else {
        assert(sym.ptr.has_value());
        const auto& val = sym.ptr.value();
        emit(opcodes::set_outer,
             {static_cast<operands::value_type>(val.level),
              static_cast<operands::value_type>(val.scope),
              static_cast<operands::value_type>(val.index)});
    }
}

void compiler::visit(const binary_expression& expr)
{
    if (expr.op == token_type::less_than) {
        expr.right->accept(*this);
        expr.left->accept(*this);
        emit(opcodes::greater_than);
        return;
    }
    expr.left->accept(*this);
    expr.right->accept(*this);
    switch (expr.op) {
        case token_type::plus:
            emit(opcodes::add);
            break;
        case token_type::minus:
            emit(opcodes::sub);
            break;
        case token_type::asterisk:
            emit(opcodes::mul);
            break;
        case token_type::slash:
            emit(opcodes::div);
            break;
        case token_type::percent:
            emit(opcodes::mod);
            break;
        case token_type::double_slash:
            emit(opcodes::floor_div);
            break;
        case token_type::ampersand:
            emit(opcodes::bit_and);
            break;
        case token_type::pipe:
            emit(opcodes::bit_or);
            break;
        case token_type::caret:
            emit(opcodes::bit_xor);
            break;
        case token_type::shift_left:
            emit(opcodes::bit_lsh);
            break;
        case token_type::shift_right:
            emit(opcodes::bit_rsh);
            break;
        case token_type::logical_and:
            emit(opcodes::logical_and);
            break;
        case token_type::logical_or:
            emit(opcodes::logical_or);
            break;
        case token_type::greater_than:
            emit(opcodes::greater_than);
            break;
        case token_type::equals:
            emit(opcodes::equal);
            break;
        case token_type::not_equals:
            emit(opcodes::not_equal);
            break;
        default:
            throw std::runtime_error(fmt::format("unsupported operator {}", expr.op));
    }
}

void compiler::visit(const boolean_literal& expr)
{
    emit(expr.value ? opcodes::tru : opcodes::fals);
}

void compiler::visit(const hash_literal& expr)
{
    for (const auto& [key, value] : expr.pairs) {
        key->accept(*this);
        value->accept(*this);
    }
    emit(opcodes::hash, expr.pairs.size() * 2);
}

void compiler::visit(const identifier& expr)
{
    auto maybe_symbol = resolve_symbol(expr.value);
    if (!maybe_symbol.has_value()) {
        throw std::runtime_error(fmt::format("undefined variable {}", expr.value));
    }
    const auto& symbol = maybe_symbol.value();
    load_symbol(symbol);
}

void compiler::visit(const if_expression& expr)
{
    expr.condition->accept(*this);
    using enum opcodes;
    auto jump_not_truthy_pos = emit(jump_not_truthy, 0);
    expr.consequence->accept(*this);
    if (last_instruction_is(pop)) {
        remove_last_pop();
    }
    auto jump_pos = emit(jump, 0);
    auto after_consequence = current_instrs().size();
    change_operand(jump_not_truthy_pos, after_consequence);

    if (expr.alternative == nullptr) {
        emit(null);
    } else {
        expr.alternative->accept(*this);
        if (last_instruction_is(pop)) {
            remove_last_pop();
        }
    }
    auto after_alternative = current_instrs().size();
    change_operand(jump_pos, after_alternative);
}

void compiler::visit(const while_statement& expr)
{
    using enum opcodes;
    auto loop_start_pos = current_instrs().size();
    expr.condition->accept(*this);
    auto jump_not_truthy_pos = emit(jump_not_truthy, 0);

    /* create a closure for the while loop body */
    enter_scope(/*inside_loop=*/true);
    expr.body->accept(*this);
    /* add a continue opcode at the end, to detect whether break was called or not */
    emit(cont);

    auto free = free_symbols();
    auto num_locals = number_symbol_definitions();
    auto instrs = leave_scope();
    for (const auto& sym : free) {
        load_symbol(sym);
    }
    auto* cmpl = make<compiled_function_object>(std::move(instrs), num_locals, 0);
    cmpl->inside_loop = true;
    auto function_index = add_constant(cmpl);
    emit(closure, {function_index, free.size()});
    emit(call, 0);

    auto jump_on_break_pos = emit(jump_not_truthy, 0);
    emit(jump, loop_start_pos);

    const auto after_body_pos = current_instrs().size();
    change_operand(jump_not_truthy_pos, after_body_pos);
    change_operand(jump_on_break_pos, after_body_pos);

    emit(null);
    emit(pop);
}

void compiler::visit(const index_expression& expr)
{
    expr.left->accept(*this);
    expr.index->accept(*this);
    emit(opcodes::index);
}

void compiler::visit(const integer_literal& expr)
{
    emit(opcodes::constant, add_constant(make<integer_object>(expr.value)));
}

void compiler::visit(const decimal_literal& expr)
{
    emit(opcodes::constant, add_constant(make<decimal_object>(expr.value)));
}

void compiler::visit(const program& expr)
{
    for (const auto& stmt : expr.statements) {
        stmt->accept(*this);
    }
}

void compiler::visit(const let_statement& expr)
{
    const auto sym = define_symbol(expr.name->value);
    expr.value->accept(*this);
    if (sym.is_local()) {
        emit(opcodes::set_local, sym.index);
    } else {
        emit(opcodes::set_global, sym.index);
    }
}

void compiler::visit(const return_statement& expr)
{
    expr.value->accept(*this);
    emit(opcodes::return_value);
}

void compiler::visit(const break_statement& /*expr*/)
{
    emit(opcodes::brake);
}

void compiler::visit(const continue_statement& /*expr*/)
{
    emit(opcodes::cont);
}

void compiler::visit(const expression_statement& expr)
{
    expr.expr->accept(*this);
    emit(opcodes::pop);
}

void compiler::visit(const block_statement& expr)
{
    for (const auto& stmt : expr.statements) {
        stmt->accept(*this);
    }
}

void compiler::visit(const string_literal& expr)
{
    emit(opcodes::constant, add_constant(make<string_object>(expr.value)));
}

void compiler::visit(const unary_expression& expr)
{
    expr.right->accept(*this);
    switch (expr.op) {
        case token_type::exclamation:
            emit(opcodes::bang);
            break;
        case token_type::minus:
            emit(opcodes::minus);
            break;
        default:
            throw std::runtime_error(fmt::format("invalid operator {}", expr.op));
    }
}

void compiler::visit(const function_literal& expr)
{
    enter_scope();
    if (!expr.name.empty()) {
        define_function_name(expr.name);
    }
    for (const auto* param : expr.parameters) {
        define_symbol(param->value);
    }
    expr.body->accept(*this);

    using enum opcodes;
    if (last_instruction_is(pop)) {
        replace_last_pop_with_return();
    }
    if (!last_instruction_is(return_value)) {
        emit(ret);
    }
    auto free = free_symbols();
    auto num_locals = number_symbol_definitions();
    auto instrs = leave_scope();
    for (const auto& sym : free) {
        load_symbol(sym);
    }
    auto function_index = add_constant(
        make<compiled_function_object>(std::move(instrs), num_locals, static_cast<int>(expr.parameters.size())));
    emit(closure, {function_index, free.size()});
}

void compiler::visit(const call_expression& expr)
{
    expr.callee->accept(*this);
    for (const auto& arg : expr.arguments) {
        arg->accept(*this);
    }
    emit(opcodes::call, expr.arguments.size());
}

namespace
{
// NOLINTBEGIN(*)
template<typename T>
auto maker(std::initializer_list<T> list) -> std::vector<T>
{
    return std::vector<T> {list};
}

template<typename T>
auto flatten(const std::vector<std::vector<T>>& arrs) -> std::vector<T>
{
    std::vector<T> result;
    for (const auto& arr : arrs) {
        std::copy(arr.cbegin(), arr.cend(), std::back_inserter(result));
    }
    return result;
}

auto check_no_parse_errors(const parser& prsr) -> bool
{
    INFO("expected no errors, got:", fmt::format("{}", fmt::join(prsr.errors(), ", ")));
    CHECK(prsr.errors().empty());
    return prsr.errors().empty();
}

using parsed_program = std::pair<program*, parser>;

auto check_program(std::string_view input) -> parsed_program
{
    auto prsr = parser {lexer {input}};
    auto prgrm = prsr.parse_program();
    INFO("while parsing: `", input, "`");
    CHECK(check_no_parse_errors(prsr));
    return {std::move(prgrm), std::move(prsr)};
}

using expected_value = std::variant<int64_t, std::string, std::vector<instructions>>;

struct ctc
{
    std::string_view input;
    std::vector<expected_value> expected_constants;
    std::vector<instructions> expected_instructions;
};

auto check_instructions(const std::vector<instructions>& instructions, const ::instructions& code)
{
    auto flattened = flatten(instructions);
    CHECK_EQ(flattened.size(), code.size());
    INFO("expected: \n", to_string(flattened), "got: \n", to_string(code));
    CHECK_EQ(flattened, code);
}

auto check_constants(const std::vector<expected_value>& expecteds, const constants& consts)
{
    CHECK_EQ(expecteds.size(), consts.size());
    for (std::size_t idx = 0; const auto& expected : expecteds) {
        const auto& actual = consts.at(idx);
        std::visit(
            overloaded {
                [&](const int64_t val) { CHECK_EQ(val, actual->as<integer_object>()->value); },
                [&](const std::string& val) { CHECK_EQ(val, actual->as<string_object>()->value); },
                [&](const std::vector<instructions>& instrs)
                { check_instructions(instrs, actual->as<compiled_function_object>()->instrs); },
            },
            expected);
        idx++;
    }
}

template<std::size_t N>
auto run(std::array<ctc, N>&& tests)
{
    for (const auto& [input, constants, instructions] : tests) {
        auto [prgrm, _] = check_program(input);
        auto cmplr = compiler::create();
        cmplr.compile(prgrm);
        check_instructions(instructions, cmplr.current_instrs());
        check_constants(constants, *cmplr.consts());
    }
}

TEST_SUITE_BEGIN("compiler");

TEST_CASE("compilerScopes")
{
    using enum opcodes;
    auto cmplr = compiler::create();
    cmplr.emit(mul);
    cmplr.enter_scope();
    cmplr.emit(sub);
    REQUIRE_EQ(cmplr.current_instrs().size(), 1);
    REQUIRE(cmplr.last_instruction_is(sub));

    cmplr.leave_scope();

    REQUIRE(cmplr.last_instruction_is(mul));
    cmplr.emit(add);
    REQUIRE_EQ(cmplr.current_instrs().size(), 2);
    REQUIRE(cmplr.last_instruction_is(add));
}

TEST_CASE("integerArithmetics")
{
    using enum opcodes;
    std::array tests {
        ctc {
            "1 + 2",
            {{1}, {2}},
            {
                make(constant, 0),
                make(constant, 1),
                make(add),
                make(pop),
            },
        },
        ctc {
            "1; 2",
            {{1}, {2}},
            {
                make(constant, 0),
                make(pop),
                make(constant, 1),
                make(pop),
            },
        },
        ctc {
            "1 - 2",
            {{1}, {2}},
            {
                make(constant, 0),
                make(constant, 1),
                make(sub),
                make(pop),
            },
        },
        ctc {
            "1 * 2",
            {{1}, {2}},
            {
                make(constant, 0),
                make(constant, 1),
                make(mul),
                make(pop),
            },
        },
        ctc {
            "1 / 2",
            {{1}, {2}},
            {
                make(constant, 0),
                make(constant, 1),
                make(div),
                make(pop),
            },
        },
        ctc {
            "1 // 2",
            {{1}, {2}},
            {
                make(constant, 0),
                make(constant, 1),
                make(floor_div),
                make(pop),
            },
        },
        ctc {
            "1 % 2",
            {{1}, {2}},
            {
                make(constant, 0),
                make(constant, 1),
                make(mod),
                make(pop),
            },
        },
        ctc {
            "1 & 2",
            {{1}, {2}},
            {
                make(constant, 0),
                make(constant, 1),
                make(bit_and),
                make(pop),
            },
        },
        ctc {
            "1 | 2",
            {{1}, {2}},
            {
                make(constant, 0),
                make(constant, 1),
                make(bit_or),
                make(pop),
            },
        },
        ctc {
            "1 ^ 2",
            {{1}, {2}},
            {
                make(constant, 0),
                make(constant, 1),
                make(bit_xor),
                make(pop),
            },
        },
        ctc {
            "1 << 2",
            {{1}, {2}},
            {
                make(constant, 0),
                make(constant, 1),
                make(bit_lsh),
                make(pop),
            },
        },
        ctc {
            "1 >> 2",
            {{1}, {2}},
            {
                make(constant, 0),
                make(constant, 1),
                make(bit_rsh),
                make(pop),
            },
        },
        ctc {
            "1 && 2",
            {{1}, {2}},
            {
                make(constant, 0),
                make(constant, 1),
                make(logical_and),
                make(pop),
            },
        },
        ctc {
            "1 || 2",
            {{1}, {2}},
            {
                make(constant, 0),
                make(constant, 1),
                make(logical_or),
                make(pop),
            },
        },
        ctc {
            "-1",
            {{1}},
            {
                make(constant, 0),
                make(minus),
                make(pop),
            },
        },
    };
    run(std::move(tests));
}

TEST_CASE("booleanExpressions")
{
    using enum opcodes;
    std::array tests {
        ctc {
            "true",
            {},
            {
                make(tru),
                make(pop),
            },
        },
        ctc {
            "false",
            {},
            {
                make(fals),
                make(pop),
            },
        },
        ctc {
            "1 > 2",
            {{1}, {2}},
            {
                make(constant, 0),
                make(constant, 1),
                make(greater_than),
                make(pop),
            },
        },
        ctc {
            "1 < 2",
            {{2}, {1}},
            {
                make(constant, 0),
                make(constant, 1),
                make(greater_than),
                make(pop),
            },
        },
        ctc {
            "1 == 2",
            {{1}, {2}},
            {
                make(constant, 0),
                make(constant, 1),
                make(equal),
                make(pop),
            },
        },
        ctc {
            "1 != 2",
            {{1}, {2}},
            {
                make(constant, 0),
                make(constant, 1),
                make(not_equal),
                make(pop),
            },
        },
        ctc {
            "true == false",
            {},
            {
                make(tru),
                make(fals),
                make(equal),
                make(pop),
            },
        },
        ctc {
            "true != false",
            {},
            {
                make(tru),
                make(fals),
                make(not_equal),
                make(pop),
            },
        },
        ctc {
            "!true",
            {},
            {
                make(tru),
                make(bang),
                make(pop),
            },
        },
        ctc {
            "true & true",
            {},
            {
                make(tru),
                make(tru),
                make(bit_and),
                make(pop),
            },
        },
        ctc {
            "true | true",
            {},
            {
                make(tru),
                make(tru),
                make(bit_or),
                make(pop),
            },
        },
        ctc {
            "true ^ true",
            {},
            {
                make(tru),
                make(tru),
                make(bit_xor),
                make(pop),
            },
        },
        ctc {
            "true << true",
            {},
            {
                make(tru),
                make(tru),
                make(bit_lsh),
                make(pop),
            },
        },
        ctc {
            "true >> true",
            {},
            {
                make(tru),
                make(tru),
                make(bit_rsh),
                make(pop),
            },
        },
        ctc {
            "true && true",
            {},
            {
                make(tru),
                make(tru),
                make(logical_and),
                make(pop),
            },
        },
        ctc {
            "true || true",
            {},
            {
                make(tru),
                make(tru),
                make(logical_or),
                make(pop),
            },
        },
    };
    run(std::move(tests));
}

TEST_CASE("conditionals")
{
    using enum opcodes;
    std::array tests {
        ctc {
            R"(if (true) { 10 }; 3333)",
            {{
                10,
                3333,
            }},
            {
                make(tru),
                make(jump_not_truthy, 10),
                make(constant, 0),
                make(jump, 11),
                make(null),
                make(pop),
                make(constant, 1),
                make(pop),
            },
        },
        ctc {
            R"(if (true) { 10 } else { 20 }; 3333)",
            {{
                10,
                20,
                3333,
            }},
            {
                make(tru),
                make(jump_not_truthy, 10),
                make(constant, 0),
                make(jump, 13),
                make(constant, 1),
                make(pop),
                make(constant, 2),
                make(pop),
            },
        },
    };
    run(std::move(tests));
}

TEST_CASE("globalLetStatements")
{
    using enum opcodes;
    std::array tests {
        ctc {
            R"(let one = 1;
               let two = 2;)",
            {{1, 2}},
            {
                make(constant, 0),
                make(set_global, 0),
                make(constant, 1),
                make(set_global, 1),
            },
        },
        ctc {
            R"(let one = 1;
               one;
        )",
            {{1}},
            {
                make(constant, 0),
                make(set_global, 0),
                make(get_global, 0),
                make(pop),
            },
        },
        ctc {
            R"(let one = 1;
               let two = one;
               two;
        )",
            {{1}},
            {
                make(constant, 0),
                make(set_global, 0),
                make(get_global, 0),
                make(set_global, 1),
                make(get_global, 1),
                make(pop),
            },
        },
    };

    run(std::move(tests));
}

TEST_CASE("stringExpression")
{
    using enum opcodes;
    std::array tests {
        ctc {
            R"("cappuchin")",
            {{"cappuchin"}},
            {
                make(constant, 0),
                make(pop),
            },
        },
        ctc {
            R"("cappu" + "chin")",
            {{"cappu", "chin"}},
            {
                make(constant, 0),
                make(constant, 1),
                make(add),
                make(pop),
            },
        },
    };
    run(std::move(tests));
}

TEST_CASE("arrayLiterals")
{
    using enum opcodes;
    std::array tests {
        ctc {
            R"([])",
            {},
            {
                make(array, 0),
                make(pop),
            },
        },
        ctc {
            R"([1, 2, 3])",
            {{1, 2, 3}},
            {
                make(constant, 0),
                make(constant, 1),
                make(constant, 2),
                make(array, 3),
                make(pop),
            },
        },
        ctc {
            R"([1 + 2, 3 - 4, 5 * 6])",
            {{1, 2, 3, 4, 5, 6}},
            {
                make(constant, 0),
                make(constant, 1),
                make(add),
                make(constant, 2),
                make(constant, 3),
                make(sub),
                make(constant, 4),
                make(constant, 5),
                make(mul),
                make(array, 3),
                make(pop),
            },
        },
        ctc {
            R"([1, 5 * 6] * 2)",
            {{1, 5, 6, 2}},
            {
                make(constant, 0),
                make(constant, 1),
                make(constant, 2),
                make(mul),
                make(array, 2),
                make(constant, 3),
                make(mul),
                make(pop),
            },
        },
        ctc {
            R"(2 * [1 + 5, 6])",
            {{2, 1, 5, 6}},
            {
                make(constant, 0),
                make(constant, 1),
                make(constant, 2),
                make(add),
                make(constant, 3),
                make(array, 2),
                make(mul),
                make(pop),
            },
        },
    };
    run(std::move(tests));
}

TEST_CASE("hashLiterals")
{
    using enum opcodes;
    std::array tests {
        ctc {
            R"({})",
            {},
            {
                make(hash, 0),
                make(pop),
            },
        },
        ctc {
            R"({1: 2, 3: 4, 5: 6 })",
            {{1, 2, 3, 4, 5, 6}},
            {
                make(constant, 0),
                make(constant, 1),
                make(constant, 2),
                make(constant, 3),
                make(constant, 4),
                make(constant, 5),
                make(hash, 6),
                make(pop),
            },
        },
        ctc {
            R"({1: 2 + 3, 4: 5 * 6})",
            {{1, 2, 3, 4, 5, 6}},
            {
                make(constant, 0),
                make(constant, 1),
                make(constant, 2),
                make(add),
                make(constant, 3),
                make(constant, 4),
                make(constant, 5),
                make(mul),
                make(hash, 4),
                make(pop),
            },
        },
    };
    run(std::move(tests));
}

TEST_CASE("indexExpression")
{
    using enum opcodes;
    std::array tests {
        ctc {
            R"([1, 2, 3][1 + 1])",
            {1, 2, 3, 1, 1},
            {
                make(constant, 0),
                make(constant, 1),
                make(constant, 2),
                make(array, 3),
                make(constant, 3),
                make(constant, 4),
                make(add),
                make(index),
                make(pop),
            },
        },
        ctc {
            R"({1: 2}[2 - 1])",
            {1, 2, 2, 1},
            {
                make(constant, 0),
                make(constant, 1),
                make(hash, 2),
                make(constant, 2),
                make(constant, 3),
                make(sub),
                make(index),
                make(pop),
            },
        },
    };
    run(std::move(tests));
}

TEST_CASE("functions")
{
    using enum opcodes;
    std::array tests {
        ctc {
            R"(fn() { return  5 + 10 })",
            {
                5,
                10,
                maker({make(constant, 0), make(constant, 1), make(add), make(return_value)}),
            },
            {
                make(closure, {2, 0}),
                make(pop),
            },
        },
        ctc {
            R"(fn() { 5 + 10 })",
            {
                5,
                10,
                maker({make(constant, 0), make(constant, 1), make(add), make(return_value)}),
            },
            {
                make(closure, {2, 0}),
                make(pop),
            },
        },
        ctc {
            R"(fn() { 1; 2})",
            {
                1,
                2,
                maker({make(constant, 0), make(pop), make(constant, 1), make(return_value)}),
            },
            {
                make(closure, {2, 0}),
                make(pop),
            },
        },
        ctc {
            R"(fn() { })",
            {
                maker({make(ret)}),
            },
            {
                make(closure, {0, 0}),
                make(pop),
            },
        },
    };
    run(std::move(tests));
}

TEST_CASE("functionCalls")
{
    using enum opcodes;
    std::array tests {
        ctc {
            R"(fn() { 24 }();)",
            {
                24,
                maker({make(constant, 0), make(return_value)}),
            },
            {
                make(closure, {1, 0}),
                make(call, 0),
                make(pop),
            },
        },
        ctc {
            R"(let noArg = fn() { 24 };
               noArg())",
            {
                24,
                maker({make(constant, 0), make(return_value)}),
            },
            {
                make(closure, {1, 0}),
                make(set_global, 0),
                make(get_global, 0),
                make(call, 0),
                make(pop),
            },
        },
        ctc {
            R"(let oneArg = fn(a) { };
               oneArg(24);)",
            {
                maker({make(ret)}),
                24,
            },
            {
                make(closure, {0, 0}),
                make(set_global, 0),
                make(get_global, 0),
                make(constant, 1),
                make(call, 1),
                make(pop),
            },
        },
        ctc {
            R"(let manyArg = fn(a, b, c) { };
               manyArg(24, 25, 26);)",
            {
                maker({make(ret)}),
                24,
                25,
                26,
            },
            {
                make(closure, {0, 0}),
                make(set_global, 0),
                make(get_global, 0),
                make(constant, 1),
                make(constant, 2),
                make(constant, 3),
                make(call, 3),
                make(pop),
            },
        },
        ctc {
            R"(let oneArg = fn(a) { a };
               oneArg(24);)",
            {
                maker({make(get_local, 0), make(return_value)}),
                24,
            },
            {
                make(closure, {0, 0}),
                make(set_global, 0),
                make(get_global, 0),
                make(constant, 1),
                make(call, 1),
                make(pop),
            },
        },
        ctc {
            R"(let manyArg = fn(a, b, c) { a; b; c; }
               manyArg(24, 25, 26);)",
            {
                maker({make(get_local, 0),
                       make(pop),
                       make(get_local, 1),
                       make(pop),
                       make(get_local, 2),
                       make(return_value)}),
                24,
                25,
                26,
            },
            {
                make(closure, {0, 0}),
                make(set_global, 0),
                make(get_global, 0),
                make(constant, 1),
                make(constant, 2),
                make(constant, 3),
                make(call, 3),
                make(pop),
            },
        },
    };
    run(std::move(tests));
}

TEST_CASE("letStatementScopes")
{
    using enum opcodes;
    std::array tests {
        ctc {
            R"(let num = 55; fn() { num };)",
            {
                55,
                maker({
                    make(get_global, 0),
                    make(return_value),
                }),
            },
            {
                make(constant, 0),
                make(set_global, 0),
                make(closure, {1, 0}),
                make(pop),
            },
        },
        ctc {
            R"(fn() {
                let num = 55;
                num 
            })",
            {
                55,
                maker({
                    make(constant, 0),
                    make(set_local, 0),
                    make(get_local, 0),
                    make(return_value),
                }),
            },
            {
                make(closure, {1, 0}),
                make(pop),
            },
        },
        ctc {
            R"(fn() {
                let a = 55;
                let b = 77;
                a + b 
            })",
            {
                55,
                77,
                maker({make(constant, 0),
                       make(set_local, 0),
                       make(constant, 1),
                       make(set_local, 1),
                       make(get_local, 0),
                       make(get_local, 1),
                       make(add),
                       make(return_value)}),
            },
            {
                make(closure, {2, 0}),
                make(pop),
            },
        },
    };
    run(std::move(tests));
}

TEST_CASE("builtins")
{
    using enum opcodes;
    std::array tests {
        ctc {
            R"(
            len([]);
            push([], 1);
            )",
            {1},
            {
                make(get_builtin, 0),
                make(array, 0),
                make(call, 1),
                make(pop),
                make(get_builtin, 5),
                make(array, 0),
                make(constant, 0),
                make(call, 2),
                make(pop),

            },
        },
        ctc {
            R"(
            fn() { len([]) }
            )",
            {maker({
                make(get_builtin, 0),
                make(array, 0),
                make(call, 1),
                make(return_value),
            })},
            {
                make(closure, {0, 0}),
                make(pop),
            },
        },
    };
    run(std::move(tests));
}

TEST_CASE("closures")
{
    using enum opcodes;
    std::array tests {
        ctc {
            R"(
            fn(a) {
                fn(b) {
                    a + b;
                }
            }
            )",
            {maker({
                 make(get_free, 0),
                 make(get_local, 0),
                 make(add),
                 make(return_value),
             }),
             maker({
                 make(get_local, 0),
                 make(closure, {0, 1}),
                 make(return_value),
             })},
            {
                make(closure, {1, 0}),
                make(pop),
            },
        },
        ctc {
            R"(
            fn(a) {
                 fn(b) {
                    fn(c) {
                        a + b + c
                    }
                }
            };)",
            {maker({
                 make(get_free, 0),
                 make(get_free, 1),
                 make(add),
                 make(get_local, 0),
                 make(add),
                 make(return_value),
             }),
             maker({
                 make(get_free, 0),
                 make(get_local, 0),
                 make(closure, {0, 2}),
                 make(return_value),
             }),
             maker({
                 make(get_local, 0),
                 make(closure, {1, 1}),
                 make(return_value),
             })},
            {
                make(closure, {2, 0}),
                make(pop),
            }},
        ctc {
            R"(
            let global = 55;
            fn() {
                let a = 66;

                fn() {
                    let b = 77;

                    fn() {
                        let c = 88;

                        global + a + b + c;
                    }
                }
            })",
            {55,
             66,
             77,
             88,
             maker({make(constant, 3),
                    make(set_local, 0),
                    make(get_global, 0),
                    make(get_free, 0),
                    make(add),
                    make(get_free, 1),
                    make(add),
                    make(get_local, 0),
                    make(add),
                    make(return_value)}),
             maker({make(constant, 2),
                    make(set_local, 0),
                    make(get_free, 0),
                    make(get_local, 0),
                    make(closure, {4, 2}),
                    make(return_value)}),
             maker({make(constant, 1),
                    make(set_local, 0),
                    make(get_local, 0),
                    make(closure, {5, 1}),
                    make(return_value)})},
            {
                make(constant, 0),
                make(set_global, 0),
                make(closure, {6, 0}),
                make(pop),
            }},
        ctc {
            R"(
            let i = 2;
            while (i > 0) {
             i = i - 1;
             let j = 3;
             let f = fn() {
                 i + j;
             }
             while (j > 0) {
                 j = j - 1;
                 puts(f() + j);
             }
            }
            )",
            {
                2,
                0,
                1,
                3,
                maker({
                    make(get_global, 0),
                    make(get_free, 0),
                    make(add),
                    make(return_value),
                }),
                0,
                1,
                maker({
                    make(get_outer, {1, 1, 0}),
                    make(constant, 6),
                    make(sub),
                    make(set_outer, {1, 1, 0}),
                    make(get_builtin, 1),
                    make(get_outer, {1, 1, 1}),
                    make(call, 0),
                    make(get_outer, {1, 1, 0}),
                    make(add),
                    make(call, 1),
                    make(pop),
                    make(cont),
                }),
                maker({
                    make(get_global, 0),
                    make(constant, 2),
                    make(sub),
                    make(set_global, 0),
                    make(constant, 3),
                    make(set_local, 0),
                    make(get_local, 0),
                    make(closure, {4, 1}),
                    make(set_local, 1),
                    make(get_local, 0),
                    make(constant, 5),
                    make(greater_than),
                    make(jump_not_truthy, 44),
                    make(closure, {7, 0}),
                    make(call, 0),
                    make(jump_not_truthy, 44),
                    make(jump, 23),
                    make(null),
                    make(pop),
                    make(cont),
                }),
            },
            {
                make(constant, 0),
                make(set_global, 0),
                make(get_global, 0),
                make(constant, 1),
                make(greater_than),
                make(jump_not_truthy, 28),
                make(closure, {8, 0}),
                make(call, 0),
                make(jump_not_truthy, 28),
                make(jump, 6),
                make(null),
                make(pop),
            }},
        ctc {
            R"(
            while (true) {
                break;
                continue;
            })",
            {
                maker({
                    make(brake),
                    make(cont),
                    make(cont),
                }),
            },
            {
                make(tru),
                make(jump_not_truthy, 16),
                make(closure, {0, 0}),
                make(call, 0),
                make(jump_not_truthy, 16),
                make(jump, 0),
                make(null),
                make(pop),
            }},
        ctc {
            R"(
            let i = 2;
            while (i > 0) {
             i = i - 1;
             let j = 3;
             let f = fn() {
                 i + j;
             }
             while (j > 0) {
                 j = j - 1;
                 puts(f() + j);
             }
            }
            )",
            {
                2,
                0,
                1,
                3,
                maker({
                    make(get_global, 0),
                    make(get_free, 0),
                    make(add),
                    make(return_value),
                }),
                0,
                1,
                maker({
                    make(get_outer, {1, 1, 0}),
                    make(constant, 6),
                    make(sub),
                    make(set_outer, {1, 1, 0}),
                    make(get_builtin, 1),
                    make(get_outer, {1, 1, 1}),
                    make(call, 0),
                    make(get_outer, {1, 1, 0}),
                    make(add),
                    make(call, 1),
                    make(pop),
                    make(cont),
                }),
                maker({
                    make(get_global, 0),
                    make(constant, 2),
                    make(sub),
                    make(set_global, 0),
                    make(constant, 3),
                    make(set_local, 0),
                    make(get_local, 0),
                    make(closure, {4, 1}),
                    make(set_local, 1),
                    make(get_local, 0),
                    make(constant, 5),
                    make(greater_than),
                    make(jump_not_truthy, 44),
                    make(closure, {7, 0}),
                    make(call, 0),
                    make(jump_not_truthy, 44),
                    make(jump, 23),
                    make(null),
                    make(pop),
                    make(cont),
                }),
            },
            {
                make(constant, 0),
                make(set_global, 0),
                make(get_global, 0),
                make(constant, 1),
                make(greater_than),
                make(jump_not_truthy, 28),
                make(closure, {8, 0}),
                make(call, 0),
                make(jump_not_truthy, 28),
                make(jump, 6),
                make(null),
                make(pop),
            }},
    };
    run(std::move(tests));
}

TEST_CASE("recursiveFunctions")
{
    using enum opcodes;
    std::array tests {
        ctc {
            R"(
        let countDown = fn(x) { countDown(x - 1); };
        countDown(1);
            )",
            {1,
             maker({make(current_closure),
                    make(get_local, 0),
                    make(constant, 0),
                    make(sub),
                    make(call, 1),
                    make(return_value)}),
             1},
            {
                make(closure, {1, 0}),
                make(set_global, 0),
                make(get_global, 0),
                make(constant, 2),
                make(call, 1),
                make(pop),
            },
        },
        ctc {
            R"(
        let wrapper = fn() {
            let countDown = fn(x) { countDown(x - 1); };
            countDown(1);
        };
        wrapper();
            )",
            {1,
             maker({make(current_closure),
                    make(get_local, 0),
                    make(constant, 0),
                    make(sub),
                    make(call, 1),
                    make(return_value)}),
             1,
             maker({
                 make(closure, {1, 0}),
                 make(set_local, 0),
                 make(get_local, 0),
                 make(constant, 2),
                 make(call, 1),
                 make(return_value),
             })},
            {
                make(closure, {3, 0}),
                make(set_global, 0),
                make(get_global, 0),
                make(call, 0),
                make(pop),
            },
        },
    };
    run(std::move(tests));
}

TEST_SUITE_END();
// NOLINTEND(*)
}  // namespace
