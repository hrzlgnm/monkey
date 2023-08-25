#include <algorithm>
#include <cstdint>
#include <iterator>
#include <map>
#include <memory>

#include "parser.hpp"

#include <ast/array_expression.hpp>
#include <ast/binary_expression.hpp>
#include <ast/boolean.hpp>
#include <ast/call_expression.hpp>
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
#include <fmt/core.h>
#include <lexer/token.hpp>

enum precedence
{
    lowest,
    equals,
    lessgreater,
    sum,
    product,
    prefix,
    call,
    idx,
};

auto precedence_of_token(token_type type) -> int
{
    switch (type) {
        case token_type::equals:
        case token_type::not_equals:
            return equals;
        case token_type::less_than:
        case token_type::greater_than:
            return lessgreater;
        case token_type::plus:
        case token_type::minus:
            return sum;
        case token_type::slash:
        case token_type::asterisk:
            return product;
        case token_type::lparen:
            return call;
        case token_type::lbracket:
            return idx;
        default:
            return lowest;
    }
}

parser::parser(lexer lxr)
    : m_lxr(lxr)
{
    next_token();
    next_token();
    using enum token_type;
    register_unary(ident, [this] { return parse_identifier(); });
    register_unary(integer, [this] { return parse_integer_literal(); });
    register_unary(exclamation, [this] { return parse_unary_expression(); });
    register_unary(minus, [this] { return parse_unary_expression(); });
    register_unary(tru, [this] { return parse_boolean(); });
    register_unary(fals, [this] { return parse_boolean(); });
    register_unary(lparen, [this] { return parse_grouped_expression(); });
    register_unary(eef, [this] { return parse_if_expression(); });
    register_unary(function, [this] { return parse_function_expression(); });
    register_unary(string, [this] { return parse_string_literal(); });
    register_unary(lbracket, [this] { return parse_array_expression(); });
    register_unary(lsquirly, [this] { return parse_hash_literal(); });
    register_binary(plus, [this](expression_ptr left) { return parse_binary_expression(std::move(left)); });
    register_binary(minus, [this](expression_ptr left) { return parse_binary_expression(std::move(left)); });
    register_binary(slash, [this](expression_ptr left) { return parse_binary_expression(std::move(left)); });
    register_binary(asterisk, [this](expression_ptr left) { return parse_binary_expression(std::move(left)); });
    register_binary(equals, [this](expression_ptr left) { return parse_binary_expression(std::move(left)); });
    register_binary(not_equals, [this](expression_ptr left) { return parse_binary_expression(std::move(left)); });
    register_binary(less_than, [this](expression_ptr left) { return parse_binary_expression(std::move(left)); });
    register_binary(greater_than, [this](expression_ptr left) { return parse_binary_expression(std::move(left)); });
    register_binary(lparen, [this](expression_ptr left) { return parse_call_expression(std::move(left)); });
    register_binary(lbracket, [this](expression_ptr left) { return parse_index_expression(std::move(left)); });
}

auto parser::parse_program() -> program_ptr
{
    auto prog = std::make_unique<program>();
    while (m_current_token.type != token_type::eof) {
        auto stmt = parse_statement();
        if (stmt) {
            prog->statements.push_back(std::move(stmt));
        }
        next_token();
    }
    return prog;
}

auto parser::errors() const -> const std::vector<std::string>&
{
    return m_errors;
}

auto parser::next_token() -> void
{
    m_current_token = m_peek_token;
    m_peek_token = m_lxr.next_token();
}

auto parser::parse_statement() -> statement_ptr
{
    using enum token_type;
    switch (m_current_token.type) {
        case let:
            return parse_let_statement();
        case ret:
            return parse_return_statement();
        default:
            return parse_expression_statement();
    }
}

auto parser::parse_let_statement() -> statement_ptr
{
    using enum token_type;
    auto stmt = std::make_unique<let_statement>();
    if (!get(ident)) {
        return {};
    }
    stmt->name = parse_identifier();

    if (!get(assign)) {
        return {};
    }

    next_token();
    stmt->value = parse_expression(lowest);
    if (auto* func_expr = dynamic_cast<function_expression*>(stmt->value.get()); func_expr != nullptr) {
        func_expr->name = stmt->name->value;
    }

    if (peek_token_is(semicolon)) {
        next_token();
    }
    return stmt;
}

auto parser::parse_return_statement() -> statement_ptr
{
    using enum token_type;
    auto stmt = std::make_unique<return_statement>();

    next_token();
    stmt->value = parse_expression(lowest);

    if (peek_token_is(semicolon)) {
        next_token();
    }
    return stmt;
}

auto parser::parse_expression_statement() -> statement_ptr
{
    auto expr_stmt = std::make_unique<expression_statement>();
    expr_stmt->expr = parse_expression(lowest);
    if (peek_token_is(token_type::semicolon)) {
        next_token();
    }
    return expr_stmt;
}

auto parser::parse_expression(int precedence) -> expression_ptr
{
    auto unary = m_unary_parsers[m_current_token.type];
    if (!unary) {
        no_unary_expression_error(m_current_token.type);
        return {};
    }
    auto left_expr = unary();
    while (!peek_token_is(token_type::semicolon) && precedence < peek_precedence()) {
        auto binary = m_binary_parsers[m_peek_token.type];
        if (!binary) {
            return left_expr;
        }
        next_token();

        left_expr = binary(std::move(left_expr));
    }
    return left_expr;
}

auto parser::parse_identifier() const -> identifier_ptr
{
    return std::make_unique<identifier>(std::string {m_current_token.literal});
}

auto parser::parse_integer_literal() -> expression_ptr
{
    auto lit = std::make_unique<integer_literal>();
    try {
        lit->value = std::stoll(std::string {m_current_token.literal});
    } catch (const std::out_of_range&) {
        new_error("could not parse {} as integer", m_current_token.literal);
        return {};
    }
    return lit;
}

auto parser::parse_unary_expression() -> expression_ptr
{
    auto pfx_expr = std::make_unique<unary_expression>();
    pfx_expr->op = m_current_token.type;

    next_token();
    pfx_expr->right = parse_expression(prefix);
    return pfx_expr;
}

auto parser::parse_boolean() -> expression_ptr
{
    return std::make_unique<boolean>(current_token_is(token_type::tru));
}

auto parser::parse_grouped_expression() -> expression_ptr
{
    next_token();
    auto exp = parse_expression(lowest);
    if (!get(token_type::rparen)) {
        return {};
    }
    return exp;
}

auto parser::parse_if_expression() -> expression_ptr
{
    using enum token_type;
    auto expr = std::make_unique<if_expression>();
    if (!get(lparen)) {
        return {};
    }
    next_token();
    expr->condition = parse_expression(lowest);

    if (!get(rparen)) {
        return {};
    }

    if (!get(lsquirly)) {
        return {};
    }

    expr->consequence = parse_block_statement();

    if (peek_token_is(elze)) {
        next_token();
        if (!get(lsquirly)) {
            return {};
        }
        expr->alternative = parse_block_statement();
    }

    return expr;
}

auto parser::parse_function_expression() -> expression_ptr
{
    using enum token_type;
    if (!get(lparen)) {
        return {};
    }
    auto parameters = parse_function_parameters();
    if (!get(lsquirly)) {
        return {};
    }
    auto body = parse_block_statement();
    return std::make_unique<function_expression>(std::move(parameters), std::move(body));
}

auto parser::parse_function_parameters() -> std::vector<std::string>
{
    using enum token_type;
    std::vector<std::string> parameters;
    if (peek_token_is(rparen)) {
        next_token();
        return parameters;
    }
    next_token();
    parameters.push_back(parse_identifier()->value);
    while (peek_token_is(comma)) {
        next_token();
        next_token();
        parameters.push_back(parse_identifier()->value);
    }
    if (!get(rparen)) {
        return {};
    }
    return parameters;
}

auto parser::parse_block_statement() -> block_statement_ptr
{
    using enum token_type;
    auto block = std::make_unique<block_statement>();
    next_token();
    while (!current_token_is(rsquirly) && !current_token_is(eof)) {
        auto stmt = parse_statement();
        if (stmt) {
            block->statements.push_back(std::move(stmt));
        }
        next_token();
    }

    return block;
}

auto parser::parse_call_expression(expression_ptr function) -> expression_ptr
{
    auto call = std::make_unique<call_expression>();
    call->function = std::move(function);
    call->arguments = parse_expression_list(token_type::rparen);
    return call;
}

auto parser::parse_binary_expression(expression_ptr left) -> expression_ptr
{
    auto bin_expr = std::make_unique<binary_expression>();
    bin_expr->op = m_current_token.type;
    bin_expr->left = std::move(left);

    auto precedence = current_precedence();
    next_token();
    bin_expr->right = parse_expression(precedence);

    return bin_expr;
}

auto parser::parse_string_literal() const -> expression_ptr
{
    return std::make_unique<string_literal>(std::string {m_current_token.literal});
}

auto parser::parse_expression_list(token_type end) -> std::vector<expression_ptr>
{
    using enum token_type;
    auto list = std::vector<expression_ptr>();
    if (peek_token_is(end)) {
        next_token();
        return list;
    }
    next_token();
    list.push_back(parse_expression(lowest));

    while (peek_token_is(comma)) {
        next_token();
        next_token();
        list.push_back(parse_expression(lowest));
    }

    if (!get(end)) {
        return {};
    }

    return list;
}

auto parser::parse_array_expression() -> expression_ptr
{
    auto array_expr = std::make_unique<array_expression>();
    array_expr->elements = parse_expression_list(token_type::rbracket);
    return array_expr;
}

auto parser::parse_index_expression(expression_ptr left) -> expression_ptr
{
    auto index_expr = std::make_unique<index_expression>();
    index_expr->left = std::move(left);
    next_token();
    index_expr->index = parse_expression(lowest);

    if (!get(token_type::rbracket)) {
        return {};
    }

    return index_expr;
}

auto parser::parse_hash_literal() -> expression_ptr
{
    auto hash = std::make_unique<hash_literal_expression>();
    using enum token_type;
    while (!peek_token_is(rsquirly)) {
        next_token();
        auto key = parse_expression(lowest);
        if (!get(colon)) {
            return {};
        }
        next_token();
        auto value = parse_expression(lowest);
        hash->pairs.emplace_back(std::move(key), std::move(value));
        if (!peek_token_is(comma)) {
            break;
        }
        next_token();
    }
    if (!get(rsquirly)) {
        return {};
    }
    return hash;
}

auto parser::get(token_type type) -> bool
{
    if (m_peek_token.type == type) {
        next_token();
        return true;
    }
    peek_error(type);
    return false;
}

auto parser::peek_error(token_type type) -> void
{
    new_error("expected next token to be {}, got {} instead", type, m_peek_token.type);
}

auto parser::register_binary(token_type type, binary_parser binary) -> void
{
    m_binary_parsers[type] = std::move(binary);
}

auto parser::register_unary(token_type type, unary_parser unary) -> void
{
    m_unary_parsers[type] = std::move(unary);
}

auto parser::current_token_is(token_type type) const -> bool
{
    return m_current_token.type == type;
}

auto parser::peek_token_is(token_type type) const -> bool
{
    return m_peek_token.type == type;
}

auto parser::no_unary_expression_error(token_type type) -> void
{
    new_error("no prefix parse function for {} found", type);
}

auto parser::peek_precedence() const -> int
{
    return precedence_of_token(m_peek_token.type);
}

auto parser::current_precedence() const -> int
{
    return precedence_of_token(m_current_token.type);
}

#include <variant>
#include <vector>

#include <doctest/doctest.h>
using expected_value_type = std::variant<int64_t, std::string, bool>;

// helper type for std::visit
template<typename... T>
struct overloaded : T...
{
    using T::operator()...;
};
template<class... T>
overloaded(T...) -> overloaded<T...>;

auto dt_assert_no_parse_errors(const parser& prsr) -> bool
{
    CHECK(prsr.errors().empty());
    return !prsr.errors().empty();
}

using parsed_program = std::pair<program_ptr, parser>;

auto dt_assert_program(std::string_view input) -> parsed_program
{
    auto prsr = parser {lexer {input}};
    auto prgrm = prsr.parse_program();
    if (dt_assert_no_parse_errors(prsr)) {
        std::cerr << "while parsing: `" << input << "`";
    };
    return {std::move(prgrm), std::move(prsr)};
}

auto dt_assert_boolean_literal(const expression_ptr& expr, bool value) -> void
{
    auto* bool_expr = dynamic_cast<boolean*>(expr.get());
    REQUIRE(bool_expr);
    REQUIRE_EQ(bool_expr->value, value);
}

auto dt_assert_identifier(const expression_ptr& expr, const std::string& value) -> void
{
    auto* ident = dynamic_cast<identifier*>(expr.get());
    REQUIRE(ident);
    REQUIRE_EQ(ident->value, value);
}

auto dt_assert_string_literal(const expression_ptr& expr, const std::string& value) -> void
{
    auto* string_lit = dynamic_cast<string_literal*>(expr.get());
    REQUIRE(string_lit);
    REQUIRE_EQ(string_lit->value, value);
}

auto dt_assert_integer_literal(const expression_ptr& expr, int64_t value) -> void
{
    auto* integer_lit = dynamic_cast<integer_literal*>(expr.get());
    REQUIRE(integer_lit);

    REQUIRE_EQ(integer_lit->value, value);
    REQUIRE_EQ(integer_lit->string(), std::to_string(value));
}

auto dt_assert_literal_expression(const expression_ptr& expr, const expected_value_type& expected) -> void
{
    std::visit(
        overloaded {
            [&](int64_t val) { dt_assert_integer_literal(expr, val); },
            [&](const std::string& val) { dt_assert_identifier(expr, val); },
            [&](bool val) { dt_assert_boolean_literal(expr, val); },
        },
        expected);
}

auto dt_assert_binary_expression(const expression_ptr& expr,
                                 const expected_value_type& left,
                                 const token_type oprtr,
                                 const expected_value_type& right) -> void
{
    auto* binary = dynamic_cast<binary_expression*>(expr.get());
    REQUIRE(binary);
    dt_assert_literal_expression(binary->left, left);
    REQUIRE_EQ(binary->op, oprtr);
    dt_assert_literal_expression(binary->right, right);
}

auto dt_assert_expression_statement(const program_ptr& prgrm) -> expression_statement*
{
    if (prgrm->statements.size() != 1) {
        throw std::invalid_argument("expected one statement, got " + std::to_string(prgrm->statements.size()));
    }
    auto* stmt = prgrm->statements[0].get();
    auto* expr_stmt = dynamic_cast<expression_statement*>(stmt);
    if (expr_stmt == nullptr) {
        throw std::invalid_argument("expected expression_statement, got " + stmt->string());
    }
    if (!expr_stmt->expr) {
        std::cout << "WARNING: expression_statement without expression found" << expr_stmt->string();
    }
    return expr_stmt;
}

template<typename E>
auto dt_assert_expression(const program_ptr& prgrm) -> E*
{
    auto* expr_stmt = dt_assert_expression_statement(prgrm);
    auto expr = dynamic_cast<E*>(expr_stmt->expr.get());
    if (!expr) {
        throw std::invalid_argument(
            fmt::format("expected {}, got {}", typeid(E).name(), prgrm->statements[0]->string()));
    }
    return expr;
}

auto dt_assert_let_statement(statement* stmt, const std::string& expected_identifier) -> let_statement*
{
    auto* let_stmt = dynamic_cast<let_statement*>(stmt);
    if (let_stmt == nullptr) {
        throw std::invalid_argument("expected let_statement, got " + stmt->string());
    }
    if (let_stmt->name->string() != expected_identifier) {
        throw std::invalid_argument("expected identifier " + expected_identifier + ", got " + let_stmt->name->string());
    }
    return let_stmt;
}

TEST_CASE("[parsing] letStatement")
{
    using enum token_type;
    auto [program, prsr] = dt_assert_program(
        R"r(let x = 5;
let y = 10;
let foobar = 838383;
)r");
    REQUIRE_EQ(program->statements.size(), 3);
    auto expected_identifiers = std::vector<std::string> {"x", "y", "foobar"};
    for (size_t i = 0; i < 3; ++i) {
        dt_assert_let_statement(program->statements[i].get(), expected_identifiers[i]);
    }
}

TEST_CASE("[parsing] letStatements")
{
    struct let_test
    {
        std::string_view input;
        std::string expected_identifier;
        expected_value_type expected_value;
    };

    std::array let_tests {
        // clang-format: off
        let_test {"let x = 5;", "x", 5},
        let_test {"let y = true;", "y", true},
        let_test {"let foobar = y;", "foobar", "y"},
        // clang-format: on
    };

    for (const auto& let : let_tests) {
        auto [prgrm, _] = dt_assert_program(let.input);
        auto* let_stmt = dt_assert_let_statement(prgrm->statements[0].get(), let.expected_identifier);
        dt_assert_literal_expression(let_stmt->value, let.expected_value);
    }
}

TEST_CASE("[parsing] parseError")
{
    using enum token_type;
    auto prsr = parser {lexer {
        R"r(let x = 5;
let y = 10;
let 838383;
)r"}};
    prsr.parse_program();
    auto errors = prsr.errors();
    CHECK_FALSE(errors.empty());
}

TEST_CASE("[parsing] returnStatement")
{
    using enum token_type;
    auto [prgrm, _] = dt_assert_program(
        R"r(return 5;
return 10;
return 993322;
)r");
    REQUIRE_EQ(prgrm->statements.size(), 3);
    std::array expected_return_values {5, 10, 993322};
    for (size_t i = 0; i < 3; ++i) {
        auto* stmt = prgrm->statements[i].get();
        auto* ret_stmt = dynamic_cast<return_statement*>(stmt);
        REQUIRE(ret_stmt);
        dt_assert_literal_expression(ret_stmt->value, expected_return_values.at(i));
    }
}

TEST_CASE("[parsing] string")
{
    using enum token_type;
    auto name = std::make_unique<identifier>("myVar");

    auto value = std::make_unique<identifier>("anotherVar");

    program prgrm;

    auto let_stmt = std::make_unique<let_statement>();
    let_stmt->name = std::move(name);
    let_stmt->value = std::move(value);
    prgrm.statements.push_back(std::move(let_stmt));

    REQUIRE_EQ(prgrm.string(), "let myVar = anotherVar;");
}

TEST_CASE("[parsing, identfierExpression")
{
    const auto* input = "foobar;";
    auto [prgrm, _] = dt_assert_program(input);
    auto* expr_stmt = dt_assert_expression_statement(prgrm);

    dt_assert_literal_expression(expr_stmt->expr, "foobar");
}

TEST_CASE("[parsing] integerExpression")
{
    auto [prgrm, _] = dt_assert_program("5;");
    auto* expr_stmt = dt_assert_expression_statement(prgrm);

    dt_assert_literal_expression(expr_stmt->expr, 5);
}

TEST_CASE("[parsing] unaryExpressions")
{
    using enum token_type;

    struct unary_test
    {
        std::string_view input;
        token_type op;
        int64_t integer_value;
    };

    std::array unary_tests {unary_test {"!5;", exclamation, 5}, unary_test {"-15;", minus, 15}};

    for (const auto& unary_test : unary_tests) {
        auto [prgrm, _] = dt_assert_program(unary_test.input);
        auto* unary = dt_assert_expression<unary_expression>(prgrm);
        REQUIRE(unary);
        REQUIRE_EQ(unary_test.op, unary->op);

        dt_assert_literal_expression(unary->right, unary_test.integer_value);
    }
}

TEST_CASE("[parsing] binaryExpressions")
{
    using enum token_type;

    struct binary_test
    {
        std::string_view input;
        int64_t left_value;
        token_type op;
        int64_t right_value;
    };

    std::array binary_tests {
        binary_test {"5 + 5;", 5, plus, 5},
        binary_test {"5 - 5;", 5, minus, 5},
        binary_test {"5 * 5;", 5, asterisk, 5},
        binary_test {"5 / 5;", 5, slash, 5},
        binary_test {"5 > 5;", 5, greater_than, 5},
        binary_test {"5 < 5;", 5, less_than, 5},
        binary_test {"5 == 5;", 5, equals, 5},
        binary_test {"5 != 5;", 5, not_equals, 5},
    };

    for (const auto& binary_test : binary_tests) {
        auto [prgrm, _] = dt_assert_program(binary_test.input);
        auto* expr_stmt = dt_assert_expression_statement(prgrm);

        dt_assert_binary_expression(expr_stmt->expr, binary_test.left_value, binary_test.op, binary_test.right_value);
    }
}

TEST_CASE("[parsing] operatorPrecedenceParsing")
{
    struct oper_test
    {
        std::string_view input;
        std::string expected;
    };

    std::array operator_precedence_tests {
        oper_test {
            "-a * b",
            "((-a) * b)",
        },
        oper_test {
            "!-a",
            "(!(-a))",
        },
        oper_test {
            "a + b + c",
            "((a + b) + c)",
        },
        oper_test {
            "a + b - c",
            "((a + b) - c)",
        },
        oper_test {
            "a * b * c",
            "((a * b) * c)",
        },
        oper_test {
            "a * b / c",
            "((a * b) / c)",
        },
        oper_test {
            "a + b / c",
            "(a + (b / c))",
        },
        oper_test {
            "a + b * c + d / e - f",
            "(((a + (b * c)) + (d / e)) - f)",
        },
        oper_test {
            "3 + 4; -5 * 5",
            "(3 + 4)((-5) * 5)",
        },
        oper_test {
            "5 > 4 == 3 < 4",
            "((5 > 4) == (3 < 4))",
        },
        oper_test {
            "5 < 4 != 3 > 4",
            "((5 < 4) != (3 > 4))",
        },
        oper_test {
            "3 + 4 * 5 == 3 * 1 + 4 * 5",
            "((3 + (4 * 5)) == ((3 * 1) + (4 * 5)))",
        },
        oper_test {
            "true",
            "true",
        },
        oper_test {
            "false",
            "false",
        },
        oper_test {
            "3 > 5 == false",
            "((3 > 5) == false)",
        },
        oper_test {
            "3 < 5 == true",
            "((3 < 5) == true)",
        },
        oper_test {
            "1 + (2 + 3) + 4",
            "((1 + (2 + 3)) + 4)",
        },
        oper_test {
            "(5 + 5) * 2",
            "((5 + 5) * 2)",
        },
        oper_test {
            "2 / (5 + 5)",
            "(2 / (5 + 5))",
        },
        oper_test {
            "(5 + 5) * 2 * (5 + 5)",
            "(((5 + 5) * 2) * (5 + 5))",
        },
        oper_test {
            "-(5 + 5)",
            "(-(5 + 5))",
        },
        oper_test {
            "!(true == true)",
            "(!(true == true))",
        },
        oper_test {
            "a + add(b * c) + d",
            "((a + add((b * c))) + d)",
        },
        oper_test {
            "add(a, b, 1, 2 * 3, 4 + 5, add(6, 7 * 8))",
            "add(a, b, 1, (2 * 3), (4 + 5), add(6, (7 * 8)))",
        },
        oper_test {
            "add(a + b + c * d / f + g)",
            "add((((a + b) + ((c * d) / f)) + g))",
        },
        oper_test {
            "a * [1, 2, 3, 4][b * c] * d",
            "((a * ([1, 2, 3, 4][(b * c)])) * d)",
        },
        oper_test {
            "add(a * b[2], b[1], 2 * [1, 2][1])",
            "add((a * (b[2])), (b[1]), (2 * ([1, 2][1])))",
        },
    };
    for (const auto& [input, expected] : operator_precedence_tests) {
        auto [prgrm, _] = dt_assert_program(input);
        CHECK_EQ(expected, prgrm->string());
    }
}

TEST_CASE("[parsing] ifExpression")
{
    const char* input = "if (x < y) { x }";
    auto [prgrm, _] = dt_assert_program(input);
    auto* if_expr = dt_assert_expression<if_expression>(prgrm);
    dt_assert_binary_expression(if_expr->condition, "x", token_type::less_than, "y");
    REQUIRE(if_expr->consequence);
    REQUIRE_EQ(if_expr->consequence->statements.size(), 1);

    auto* consequence = dynamic_cast<expression_statement*>(if_expr->consequence->statements[0].get());
    REQUIRE(consequence);
    dt_assert_identifier(consequence->expr, "x");
    CHECK_FALSE(if_expr->alternative);
}

TEST_CASE("[parsing] ifElseExpression")
{
    const char* input = "if (x < y) { x } else { y }";
    auto [prgrm, _] = dt_assert_program(input);
    auto* if_expr = dt_assert_expression<if_expression>(prgrm);

    dt_assert_binary_expression(if_expr->condition, "x", token_type::less_than, "y");
    REQUIRE(if_expr->consequence);
    REQUIRE_EQ(if_expr->consequence->statements.size(), 1);

    auto* consequence = dynamic_cast<expression_statement*>(if_expr->consequence->statements[0].get());
    REQUIRE(consequence);
    dt_assert_identifier(consequence->expr, "x");

    auto* alternative = dynamic_cast<expression_statement*>(if_expr->alternative->statements[0].get());
    REQUIRE(alternative);
    dt_assert_identifier(alternative->expr, "y");
}

TEST_CASE("[parsing] functionLiteral")
{
    const char* input = "fn(x, y) { x + y; }";
    auto [prgrm, _] = dt_assert_program(input);
    auto* fn_expr = dt_assert_expression<function_expression>(prgrm);

    REQUIRE_EQ(fn_expr->parameters.size(), 2);

    REQUIRE_EQ(fn_expr->parameters[0], "x");
    REQUIRE_EQ(fn_expr->parameters[1], "y");

    auto* block = dynamic_cast<block_statement*>(fn_expr->body.get());
    REQUIRE_EQ(block->statements.size(), 1);

    auto* body_stmt = dynamic_cast<expression_statement*>(block->statements.at(0).get());
    REQUIRE(body_stmt);
    dt_assert_binary_expression(body_stmt->expr, "x", token_type::plus, "y");
}

TEST_CASE("[parsing] functionLiteralWithName")
{
    const auto* input = R"(let myFunction = fn() { };)";
    auto [prgrm, _] = dt_assert_program(input);
    auto* let = dt_assert_let_statement(prgrm->statements[0].get(), "myFunction");
    auto* fnexpr = dynamic_cast<function_expression*>(let->value.get());
    REQUIRE(fnexpr);
    REQUIRE_EQ(fnexpr->name, "myFunction");
}

TEST_CASE("[parsing] functionParameters")
{
    struct parameters_test
    {
        std::string_view input;
        std::vector<std::string> expected;
    };

    std::array parameter_tests {
        parameters_test {"fn() {};", {}},
        parameters_test {"fn(x) {};", {"x"}},
        parameters_test {"fn(x, y, z) {};", {"x", "y", "z"}},
    };
    for (const auto& [input, expected] : parameter_tests) {
        auto [prgrm, _] = dt_assert_program(input);
        auto* fn_expr = dt_assert_expression<function_expression>(prgrm);

        REQUIRE_EQ(fn_expr->parameters.size(), expected.size());
        for (size_t index = 0; const auto& val : expected) {
            REQUIRE_EQ(fn_expr->parameters[index], val);
            ++index;
        }
    }
}

TEST_CASE("[parsing] callExpressionParsing")
{
    const auto* input = "add(1, 2 * 3, 4 + 5);";
    auto [prgrm, _] = dt_assert_program(input);
    auto* call = dt_assert_expression<call_expression>(prgrm);
    dt_assert_identifier(call->function, "add");
    REQUIRE_EQ(call->arguments.size(), 3);
    dt_assert_literal_expression(call->arguments[0], 1);
    dt_assert_binary_expression(call->arguments[1], 2, token_type::asterisk, 3);
    dt_assert_binary_expression(call->arguments[2], 4, token_type::plus, 5);
}

TEST_CASE("[parsing] stringLiteralExpression")
{
    const auto* input = "\"hello world\";";
    auto [prgrm, _] = dt_assert_program(input);
    auto* str = dt_assert_expression<string_literal>(prgrm);
    REQUIRE_EQ(str->value, "hello world");
}

TEST_CASE("[parsing] arrayExpression")
{
    auto [prgrm, _] = dt_assert_program("[1, 2 * 2, 3 + 3]");
    auto* array_expr = dt_assert_expression<array_expression>(prgrm);
    REQUIRE_EQ(array_expr->elements.size(), 3);
    dt_assert_integer_literal(array_expr->elements[0], 1);
    dt_assert_binary_expression(array_expr->elements[1], 2, token_type::asterisk, 2);
    dt_assert_binary_expression(array_expr->elements[2], 3, token_type::plus, 3);
}

TEST_CASE("[parsing] indexEpxression")
{
    auto [prgrm, _] = dt_assert_program("myArray[1+1]");
    auto* idx_expr = dt_assert_expression<index_expression>(prgrm);
    dt_assert_identifier(idx_expr->left, "myArray");
    dt_assert_binary_expression(idx_expr->index, 1, token_type::plus, 1);
}

TEST_CASE("[parsing] hashLiteralStringKeys")
{
    auto [prgrm, _] = dt_assert_program(R"({"one": 1, "two": 2, "three": 3})");
    auto* hash_lit = dt_assert_expression<hash_literal_expression>(prgrm);
    std::array keys {"one", "two", "three"};
    std::array values {1, 2, 3};
    for (auto idx = 0UL; const auto& [k, v] : hash_lit->pairs) {
        dt_assert_string_literal(k, keys.at(idx));
        dt_assert_integer_literal(v, values.at(idx));
        idx++;
    }
}

TEST_CASE("[parsing] hashLiteralWithExpression")
{
    auto [prgrm, _] = dt_assert_program(R"({"one": 0 + 1, "two": 10 - 8, "three": 15 / 5})");
    auto* hash_lit = dt_assert_expression<hash_literal_expression>(prgrm);
    std::array keys {"one", "two", "three"};

    struct test
    {
        int64_t left;
        token_type oper;
        int64_t right;
    };

    std::array expected {
        test {0, token_type::plus, 1}, test {10, token_type::minus, 8}, test {15, token_type::slash, 5}};
    for (size_t idx = 0; const auto& [k, v] : hash_lit->pairs) {
        dt_assert_string_literal(k, keys.at(idx));
        dt_assert_binary_expression(v, expected.at(idx).left, expected.at(idx).oper, expected.at(idx).right);
        idx++;
    }
}

TEST_CASE("[parsing] emptyHashLiteral")
{
    auto [prgrm, _] = dt_assert_program(R"({})");
    auto* hash_lit = dt_assert_expression<hash_literal_expression>(prgrm);
    REQUIRE(hash_lit->pairs.empty());
}
