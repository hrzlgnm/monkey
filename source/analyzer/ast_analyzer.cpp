#include <stdexcept>
#include <string_view>

#include <analyzer/analyzer.hpp>
#include <ast/array_expression.hpp>
#include <ast/assign_expression.hpp>
#include <ast/binary_expression.hpp>
#include <ast/call_expression.hpp>
#include <ast/expression.hpp>
#include <ast/function_expression.hpp>
#include <ast/hash_literal_expression.hpp>
#include <ast/identifier.hpp>
#include <ast/if_expression.hpp>
#include <ast/index_expression.hpp>
#include <ast/program.hpp>
#include <ast/statements.hpp>
#include <ast/string_literal.hpp>
#include <ast/unary_expression.hpp>
#include <compiler/symbol_table.hpp>
#include <doctest/doctest.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <lexer/lexer.hpp>
#include <parser/parser.hpp>

auto array_expression::check(symbol_table* symbols) const -> void
{
    for (const auto* element : elements) {
        element->check(symbols);
    }
}

auto assign_expression::check(symbol_table* symbols) const -> void
{
    auto maybe_symbol = symbols->resolve(name->value);
    if (!maybe_symbol.has_value()) {
        fail(fmt::format("identifier not found: {}", name->value));
    }
    const auto& symbol = maybe_symbol.value();
    if (symbol.is_function() || (symbol.is_outer() && symbol.ptr.value().is_function())) {
        fail(fmt::format("cannot reassign the current function being defined: {}", name->value));
    }
    value->check(symbols);
}

auto binary_expression::check(symbol_table* symbols) const -> void
{
    left->check(symbols);
    right->check(symbols);
}

auto hash_literal_expression::check(symbol_table* symbols) const -> void
{
    for (const auto& [key, value] : pairs) {
        key->check(symbols);
        value->check(symbols);
    }
}

auto identifier::check(symbol_table* symbols) const -> void
{
    auto symbol = symbols->resolve(value);
    if (!symbol.has_value()) {
        fail(fmt::format("identifier not found: {}", value));
    }
}

auto if_expression::check(symbol_table* symbols) const -> void
{
    condition->check(symbols);
    consequence->check(symbols);
    if (alternative != nullptr) {
        alternative->check(symbols);
    }
}

auto while_statement::check(symbol_table* symbols) const -> void
{
    condition->check(symbols);

    auto* inner = symbol_table::create_enclosed(symbols, /*inside_loop=*/true);
    body->check(inner);
}

auto index_expression::check(symbol_table* symbols) const -> void
{
    left->check(symbols);
    index->check(symbols);
}

auto program::check(symbol_table* symbols) const -> void
{
    for (const auto* statement : statements) {
        statement->check(symbols);
    }
}

auto let_statement::check(symbol_table* symbols) const -> void
{
    auto symbol = symbols->resolve(name->value);
    if (symbol.has_value()) {
        const auto& value = symbol.value();
        if (value.is_local() || (value.is_global() && symbol->is_global())) {
            fail(fmt::format("{} is already defined", name->value));
        }
    }
    symbols->define(name->value);
    value->check(symbols);
}

auto return_statement::check(symbol_table* symbols) const -> void
{
    value->check(symbols);
}

auto break_statement::check(symbol_table* symbols) const -> void
{
    if (!symbols->inside_loop()) {
        fail("syntax error: break outside loop");
    }
}

auto continue_statement::check(symbol_table* symbols) const -> void
{
    if (!symbols->inside_loop()) {
        fail("syntax error: continue outside loop");
    }
}

auto expression_statement::check(symbol_table* symbols) const -> void
{
    expr->check(symbols);
}

auto block_statement::check(symbol_table* symbols) const -> void
{
    for (const auto* statement : statements) {
        statement->check(symbols);
    }
}

auto unary_expression::check(symbol_table* symbols) const -> void
{
    right->check(symbols);
}

auto function_expression::check(symbol_table* symbols) const -> void
{
    auto* inner = symbol_table::create_enclosed(symbols);
    if (!name.empty()) {
        inner->define_function_name(name);
    }

    for (const auto& parameter : parameters) {
        inner->define(parameter);
    }

    body->check(inner);
}

auto call_expression::check(symbol_table* symbols) const -> void
{
    function->check(symbols);
    for (const auto* arg : arguments) {
        arg->check(symbols);
    }
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
    analyze_program(prgrm, nullptr);
}

TEST_SUITE("analyzer")
{
    TEST_CASE("analyze_program")
    {
        struct test
        {
            std::string_view input;
            std::string_view expected_exception_string;
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
        };
        for (const auto& test : tests) {
            INFO(test.input, " expected error: ", test.expected_exception_string);
            CHECK_THROWS_WITH_AS(analyze(test.input), test.expected_exception_string.data(), std::runtime_error);
        }
    }
}

// NOLINTEND(*)
}  // namespace
