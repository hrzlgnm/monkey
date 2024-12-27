#include <stdexcept>
#include <string>

#include "analyzer.hpp"

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
#include <compiler/symbol_table.hpp>
#include <doctest/doctest.h>
#include <eval/environment.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <lexer/lexer.hpp>
#include <object/object.hpp>
#include <parser/parser.hpp>

namespace
{
[[noreturn]] void fail(const std::string& error_message)
{
    throw std::runtime_error(error_message);
}
}  // namespace

void analyze_program(const program* program,
                     symbol_table* existing_symbols,
                     const environment* existing_env) noexcept(false)
{
    symbol_table* symbols = nullptr;
    if (existing_symbols != nullptr) {
        symbols = symbol_table::create_enclosed(existing_symbols);
    } else {
        symbols = symbol_table::create();
        for (auto i = 0; const auto* builtin : builtin::builtins()) {
            symbols->define_builtin(i++, builtin->name);
        }
    }
    if (existing_env != nullptr) {
        for (const auto& [key, _] : existing_env->store) {
            symbols->define(key);
        }
    }
    analyzer an {symbols};
    an.analyze(program);
}
using enum object::object_type;

analyzer::analyzer(symbol_table* symbols)
    : m_symbols {symbols}
{
}

void analyzer::analyze(const program* prgrm)
{
    prgrm->accept(*this);
}

void analyzer::visit(const array_literal& expr)
{
    for (const auto* element : expr.elements) {
        element->accept(*this);
    }
    see(array);
}

void analyzer::visit(const assign_expression& expr)
{
    auto maybe_symbol = m_symbols->resolve(expr.name->value);
    if (!maybe_symbol.has_value()) {
        fail(fmt::format("identifier not found: {}", expr.name->value));
    }
    const auto& symbol = maybe_symbol.value();
    // NOLINTBEGIN(bugprone-unchecked-optional-access)
    if (symbol.is_function() || (symbol.is_outer() && symbol.ptr.value().is_function())) {
        fail(fmt::format("cannot reassign the current function being defined: {}", expr.name->value));
    }
    // NOLINTEND(bugprone-unchecked-optional-access)
    expr.value->accept(*this);
    if (m_last_type.has_value()) {
        see(symbol.name, m_last_type.value());
    }
}

void analyzer::visit(const binary_expression& expr)
{
    expr.left->accept(*this);
    expr.right->accept(*this);
}

void analyzer::visit(const hash_literal& expr)
{
    for (const auto& [key, value] : expr.pairs) {
        key->accept(*this);
        value->accept(*this);
    }
    see(hash);
}

void analyzer::visit(const identifier& expr)
{
    auto maybe_symbol = m_symbols->resolve(expr.value);
    if (!maybe_symbol.has_value()) {
        fail(fmt::format("identifier not found: {}", expr.value));
    }
    const auto& symbol = maybe_symbol.value();
    if (symbol.is_builtin()) {
        see(expr.value, builtin);
    } else if (symbol.is_function()) {
        see(expr.value, function);
    } else {
        see(expr.value);
    }
}

void analyzer::visit(const if_expression& expr)
{
    expr.condition->accept(*this);
    expr.consequence->accept(*this);
    if (expr.alternative != nullptr) {
        expr.alternative->accept(*this);
    }
}

void analyzer::visit(const while_statement& expr)
{
    expr.condition->accept(*this);

    auto* inner = symbol_table::create_enclosed(m_symbols, /*inside_loop=*/true);
    analyzer w {inner};
    expr.body->accept(w);
}

void analyzer::visit(const index_expression& expr)
{
    expr.left->accept(*this);
    expr.index->accept(*this);
}

void analyzer::visit(const program& expr)
{
    for (const auto* statement : expr.statements) {
        statement->accept(*this);
    }
}

void analyzer::visit(const let_statement& expr)
{
    auto symbol = m_symbols->resolve(expr.name->value);
    if (symbol.has_value()) {
        const auto& value = symbol.value();
        if (value.is_local() || (value.is_global() && m_symbols->is_global())) {
            fail(fmt::format("{} is already defined", expr.name->value));
        }
    }
    m_symbols->define(expr.name->value);
    expr.value->accept(*this);
    if (m_last_type.has_value()) {
        see(expr.name->value, m_last_type.value());
    }
}

void analyzer::visit(const return_statement& expr)
{
    expr.value->accept(*this);
}

void analyzer::visit(const break_statement& /*expr*/)
{
    if (!m_symbols->inside_loop()) {
        fail("syntax error: break outside loop");
    }
}

void analyzer::visit(const continue_statement& /*expr*/)
{
    if (!m_symbols->inside_loop()) {
        fail("syntax error: continue outside loop");
    }
}

void analyzer::visit(const expression_statement& expr)
{
    expr.expr->accept(*this);
}

void analyzer::visit(const block_statement& expr)
{
    for (const auto* statement : expr.statements) {
        statement->accept(*this);
    }
}

void analyzer::visit(const unary_expression& expr)
{
    expr.right->accept(*this);
}

void analyzer::visit(const function_literal& expr)
{
    auto* inner = symbol_table::create_enclosed(m_symbols);
    if (!expr.name.empty()) {
        inner->define_function_name(expr.name);
        see(expr.name, function);
    }

    for (const auto* parameter : expr.parameters) {
        inner->define(parameter->value);
    }
    analyzer f(inner);
    expr.body->accept(f);
}

void analyzer::visit(const call_expression& expr)
{
    expr.callee->accept(*this);
    if (m_last_type != function && m_last_type != builtin) {
        fail(fmt::format("type error: `{}` is not callable", m_last_type.value()));
    }

    for (const auto* arg : expr.arguments) {
        arg->accept(*this);
    }
}

void analyzer::visit(const boolean_literal& /* expr */)
{
    see(object::object_type::boolean);
}

void analyzer::visit(const decimal_literal& /* expr */)
{
    see(object::object_type::decimal);
}

void analyzer::visit(const integer_literal& /* expr */)
{
    see(object::object_type::integer);
}

void analyzer::visit(const string_literal& /* expr */)
{
    see(object::object_type::string);
}

void analyzer::see(const std::string& identifier, object::object_type type)
{
    m_last_type = m_symbol_types[identifier] = type;
}

void analyzer::see(const std::string& identifier)
{
    m_last_type = m_symbol_types[identifier];
}

void analyzer::see(object::object_type type)
{
    m_last_type = type;
}

namespace
{
// NOLINTBEGIN(*)
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
    auto* prgrm = prsr.parse_program();
    INFO("while parsing: `", input, "`");
    CHECK(check_no_parse_errors(prsr));
    return {prgrm, std::move(prsr)};
}

auto analyze(std::string_view input) noexcept(false) -> void
{
    auto [prgrm, _] = check_program(input);
    analyze_program(prgrm, nullptr, nullptr);
}

TEST_SUITE("analyzer")
{
    TEST_CASE("analyze_program")
    {
        struct test
        {
            std::string_view input;
            std::string expected_exception_string;
        };

        std::array tests {
            test {.input = "foobar", .expected_exception_string = "identifier not found: foobar"},
            test {.input = "x = 2;", .expected_exception_string = "identifier not found: x"},
            test {.input = "let a = 2; let a = 4;", .expected_exception_string = "a is already defined"},
            test {.input = "let f = fn(x) { let x = 3; }", .expected_exception_string = "x is already defined"},
            test {.input = "break;", .expected_exception_string = "syntax error: break outside loop"},
            test {.input = "continue;", .expected_exception_string = "syntax error: continue outside loop"},
            test {.input = "fn () { break; } ", .expected_exception_string = "syntax error: break outside loop"},
            test {.input = "fn () { continue; } ", .expected_exception_string = "syntax error: continue outside loop"},
            test {.input = "while (x == 2) {}", .expected_exception_string = "identifier not found: x"},
            test {.input = "while (true) { x = 2; }", .expected_exception_string = "identifier not found: x"},
            test {.input = "if (x == 2) {}", .expected_exception_string = "identifier not found: x"},
            test {.input = "if (true) { x }", .expected_exception_string = "identifier not found: x"},
            test {.input = "if (true) { 2 } else { x }", .expected_exception_string = "identifier not found: x"},
            test {.input = "-x", .expected_exception_string = "identifier not found: x"},
            test {.input = "x + 2", .expected_exception_string = "identifier not found: x"},
            test {.input = "x(3)", .expected_exception_string = "identifier not found: x"},
            test {.input = "x[3]", .expected_exception_string = "identifier not found: x"},
            test {.input = "[1, 2][x]", .expected_exception_string = "identifier not found: x"},
            test {.input = "len(x)", .expected_exception_string = "identifier not found: x"},
            test {.input = "[x]", .expected_exception_string = "identifier not found: x"},
            test {.input = "{x: 2}", .expected_exception_string = "identifier not found: x"},
            test {.input = "{2: x}", .expected_exception_string = "identifier not found: x"},
            test {.input = "let f = fn(x) { if (x > 0) { f(x - 1); f = 2; } }",
                  .expected_exception_string = "cannot reassign the current function being defined: f"},
            test {.input = "1(2);", .expected_exception_string = "type error: `integer` is not callable"},
            test {.input = "let i = 1; i(2);", .expected_exception_string = "type error: `integer` is not callable"},
        };
        for (const auto& test : tests) {
            INFO("with: `", test.input, "` expected error: ", test.expected_exception_string);
            CHECK_THROWS_WITH_AS(analyze(test.input), test.expected_exception_string.c_str(), std::runtime_error);
        }
    }
}

// NOLINTEND(*)
}  // namespace
