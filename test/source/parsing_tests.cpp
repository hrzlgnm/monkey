#include <gtest/gtest.h>

#include "array_expression.hpp"
#include "binary_expression.hpp"
#include "boolean.hpp"
#include "call_expression.hpp"
#include "function_expression.hpp"
#include "hash_literal_expression.hpp"
#include "if_expression.hpp"
#include "index_expression.hpp"
#include "integer_literal.hpp"
#include "parser.hpp"
#include "program.hpp"
#include "string_literal.hpp"
#include "testutils.hpp"
#include "unary_expression.hpp"

template<class>
inline constexpr bool always_false_v {false};

using expected_value_type = std::variant<int64_t, std::string, bool>;

auto assert_boolean_literal(const expression_ptr& expr, bool value) -> void
{
    auto* bool_expr = dynamic_cast<boolean*>(expr.get());
    ASSERT_TRUE(bool_expr) << "expected boolean, got" << expr.get();
    ASSERT_EQ(bool_expr->value, value);
}

auto assert_identifier(const expression_ptr& expr, const std::string& value) -> void
{
    auto* ident = dynamic_cast<identifier*>(expr.get());
    ASSERT_TRUE(ident) << "expected identifier, got" << expr.get();
    ASSERT_EQ(ident->value, value);
}

auto assert_string_literal(const expression_ptr& expr, const std::string& value) -> void
{
    auto* string_lit = dynamic_cast<string_literal*>(expr.get());
    ASSERT_TRUE(string_lit) << "expected string_literal, got " << expr.get()->string();

    ASSERT_EQ(string_lit->value, value);
}

auto assert_integer_literal(const expression_ptr& expr, int64_t value) -> void
{
    auto* integer_lit = dynamic_cast<integer_literal*>(expr.get());
    ASSERT_TRUE(integer_lit) << "expected integer_literal, got " << expr.get();

    ASSERT_EQ(integer_lit->value, value);
    ASSERT_EQ(integer_lit->string(), std::to_string(value));
}

auto assert_literal_expression(const expression_ptr& expr, const expected_value_type& expected) -> void
{
    std::visit(
        [&](auto&& arg)
        {
            using t = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<t, int64_t>) {
                assert_integer_literal(expr, arg);
            } else if constexpr (std::is_same_v<t, std::string>) {
                assert_identifier(expr, arg);
            } else if constexpr (std::is_same_v<t, bool>) {
                assert_boolean_literal(expr, arg);
            } else {
                static_assert(always_false_v<t>, "non-exhaustive visitor!");
            }
        },
        expected);
}

auto assert_binary_expression(const expression_ptr& expr,
                              const expected_value_type& left,
                              const token_type oprtr,
                              const expected_value_type& right) -> void
{
    auto* binary = dynamic_cast<binary_expression*>(expr.get());
    ASSERT_TRUE(binary);
    assert_literal_expression(binary->left, left);
    ASSERT_EQ(binary->op, oprtr);
    assert_literal_expression(binary->right, right);
}

auto assert_expression_statement(const program_ptr& prgrm) -> expression_statement*
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
auto assert_expression(const program_ptr& prgrm) -> E*
{
    auto* expr_stmt = assert_expression_statement(prgrm);
    auto expr = dynamic_cast<E*>(expr_stmt->expr.get());
    if (!expr) {
        throw std::invalid_argument(
            fmt::format("expected {}, got {}", typeid(E).name(), prgrm->statements[0]->string()));
    }
    return expr;
}

auto assert_let_statement(statement* stmt, const std::string& expected_identifier) -> let_statement*
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

// NOLINTBEGIN(*-magic-numbers)
TEST(parsing, testLetStatement)
{
    using enum token_type;
    auto [program, prsr] = assert_program(
        R"r(let x = 5;
let y = 10;
let foobar = 838383;
)r");
    ASSERT_EQ(program->statements.size(), 3);
    auto expected_identifiers = std::vector<std::string> {"x", "y", "foobar"};
    for (size_t i = 0; i < 3; ++i) {
        assert_let_statement(program->statements[i].get(), expected_identifiers[i]);
    }
}

TEST(parsing, testLetStatements)
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
        auto [prgrm, _] = assert_program(let.input);
        auto* let_stmt = assert_let_statement(prgrm->statements[0].get(), let.expected_identifier);
        assert_literal_expression(let_stmt->value, let.expected_value);
    }
}

TEST(parsing, parseError)
{
    using enum token_type;
    auto prsr = parser {lexer {
        R"r(let x = 5;
let y = 10;
let 838383;
)r"}};
    prsr.parse_program();
    auto errors = prsr.errors();
    ASSERT_FALSE(errors.empty());
}

TEST(parsing, testReturnStatement)
{
    using enum token_type;
    auto [prgrm, _] = assert_program(
        R"r(return 5;
return 10;
return 993322;
)r");
    ASSERT_EQ(prgrm->statements.size(), 3);
    std::array expected_return_values {5, 10, 993322};
    for (size_t i = 0; i < 3; ++i) {
        auto* stmt = prgrm->statements[i].get();
        auto* ret_stmt = dynamic_cast<return_statement*>(stmt);
        ASSERT_TRUE(ret_stmt);
        assert_literal_expression(ret_stmt->value, expected_return_values.at(i));
    }
}

TEST(parsing, testString)
{
    using enum token_type;
    auto name = std::make_unique<identifier>("myVar");

    auto value = std::make_unique<identifier>("anotherVar");

    program prgrm;

    auto let_stmt = std::make_unique<let_statement>();
    let_stmt->name = std::move(name);
    let_stmt->value = std::move(value);
    prgrm.statements.push_back(std::move(let_stmt));

    ASSERT_EQ(prgrm.string(), "let myVar = anotherVar;");
}

TEST(parsing, testIdentfierExpression)
{
    const auto* input = "foobar;";
    auto [prgrm, _] = assert_program(input);
    auto* expr_stmt = assert_expression_statement(prgrm);

    assert_literal_expression(expr_stmt->expr, "foobar");
}

TEST(parsing, testIntegerExpression)
{
    auto [prgrm, _] = assert_program("5;");
    auto* expr_stmt = assert_expression_statement(prgrm);

    assert_literal_expression(expr_stmt->expr, 5);
}

TEST(parsing, testUnaryExpressions)
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
        auto [prgrm, _] = assert_program(unary_test.input);
        auto* unary = assert_expression<unary_expression>(prgrm);
        ASSERT_TRUE(unary);
        ASSERT_EQ(unary_test.op, unary->op);

        assert_literal_expression(unary->right, unary_test.integer_value);
    }
}

TEST(parsing, testBinaryExpressions)
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
        auto [prgrm, _] = assert_program(binary_test.input);
        auto* expr_stmt = assert_expression_statement(prgrm);

        assert_binary_expression(expr_stmt->expr, binary_test.left_value, binary_test.op, binary_test.right_value);
    }
}

TEST(parsing, testOperatorPrecedenceParsing)
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
        auto [prgrm, _] = assert_program(input);
        EXPECT_EQ(expected, prgrm->string()) << "with input " << input;
    }
}

TEST(parsing, testIfExpression)
{
    const char* input = "if (x < y) { x }";
    auto [prgrm, _] = assert_program(input);
    auto* if_expr = assert_expression<if_expression>(prgrm);
    assert_binary_expression(if_expr->condition, "x", token_type::less_than, "y");
    ASSERT_TRUE(if_expr->consequence);
    ASSERT_EQ(if_expr->consequence->statements.size(), 1);

    auto* consequence = dynamic_cast<expression_statement*>(if_expr->consequence->statements[0].get());
    ASSERT_TRUE(consequence);
    assert_identifier(consequence->expr, "x");
    ASSERT_FALSE(if_expr->alternative);
}

TEST(parsing, testIfElseExpression)
{
    const char* input = "if (x < y) { x } else { y }";
    auto [prgrm, _] = assert_program(input);
    auto* if_expr = assert_expression<if_expression>(prgrm);

    assert_binary_expression(if_expr->condition, "x", token_type::less_than, "y");
    ASSERT_TRUE(if_expr->consequence);
    ASSERT_EQ(if_expr->consequence->statements.size(), 1);

    auto* consequence = dynamic_cast<expression_statement*>(if_expr->consequence->statements[0].get());
    ASSERT_TRUE(consequence);
    assert_identifier(consequence->expr, "x");

    auto* alternative = dynamic_cast<expression_statement*>(if_expr->alternative->statements[0].get());
    ASSERT_TRUE(alternative);
    assert_identifier(alternative->expr, "y");
}

TEST(parsing, testFunctionLiteral)
{
    const char* input = "fn(x, y) { x + y; }";
    auto [prgrm, _] = assert_program(input);
    auto* fn_expr = assert_expression<function_expression>(prgrm);

    ASSERT_EQ(fn_expr->parameters.size(), 2);

    ASSERT_EQ(fn_expr->parameters[0], "x");
    ASSERT_EQ(fn_expr->parameters[1], "y");

    auto* block = dynamic_cast<block_statement*>(fn_expr->body.get());
    ASSERT_EQ(block->statements.size(), 1);
    auto* body_stmt = dynamic_cast<expression_statement*>(block->statements.at(0).get());

    assert_binary_expression(body_stmt->expr, "x", token_type::plus, "y");
}

TEST(parsing, testFunctionParameters)
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
        auto [prgrm, _] = assert_program(input);
        auto* fn_expr = assert_expression<function_expression>(prgrm);

        ASSERT_EQ(fn_expr->parameters.size(), expected.size());
        for (size_t index = 0; const auto& val : expected) {
            ASSERT_EQ(fn_expr->parameters[index], val);
            ++index;
        }
    }
}

TEST(parsing, testCallExpressionParsing)
{
    const auto* input = "add(1, 2 * 3, 4 + 5);";
    auto [prgrm, _] = assert_program(input);
    auto* call = assert_expression<call_expression>(prgrm);
    assert_identifier(call->function, "add");
    ASSERT_EQ(call->arguments.size(), 3);
    assert_literal_expression(call->arguments[0], 1);
    assert_binary_expression(call->arguments[1], 2, token_type::asterisk, 3);
    assert_binary_expression(call->arguments[2], 4, token_type::plus, 5);
}

TEST(parsing, testStringLiteralExpression)
{
    const auto* input = "\"hello world\";";
    auto [prgrm, _] = assert_program(input);
    auto* str = assert_expression<string_literal>(prgrm);
    ASSERT_EQ(str->value, "hello world");
}

TEST(parsing, testArrayExpression)
{
    auto [prgrm, _] = assert_program("[1, 2 * 2, 3 + 3]");
    auto* array_expr = assert_expression<array_expression>(prgrm);
    ASSERT_EQ(array_expr->elements.size(), 3);
    assert_integer_literal(array_expr->elements[0], 1);
    assert_binary_expression(array_expr->elements[1], 2, token_type::asterisk, 2);
    assert_binary_expression(array_expr->elements[2], 3, token_type::plus, 3);
}

TEST(parsing, testIndexEpxression)
{
    auto [prgrm, _] = assert_program("myArray[1+1]");
    auto* idx_expr = assert_expression<index_expression>(prgrm);
    assert_identifier(idx_expr->left, "myArray");
    assert_binary_expression(idx_expr->index, 1, token_type::plus, 1);
}

TEST(parsing, testHashLiteralStringKeys)
{
    auto [prgrm, _] = assert_program(R"({"one": 1, "two": 2, "three": 3})");
    auto* hash_lit = assert_expression<hash_literal_expression>(prgrm);
    std::array keys {"one", "two", "three"};
    std::array values {1, 2, 3};
    for (auto idx = 0UL; const auto& [k, v] : hash_lit->pairs) {
        assert_string_literal(k, keys.at(idx));
        assert_integer_literal(v, values.at(idx));
        idx++;
    }
}

TEST(parsing, testHashLiteralWithExpression)
{
    auto [prgrm, _] = assert_program(R"({"one": 0 + 1, "two": 10 - 8, "three": 15 / 5})");
    auto* hash_lit = assert_expression<hash_literal_expression>(prgrm);
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
        assert_string_literal(k, keys.at(idx));
        assert_binary_expression(v, expected.at(idx).left, expected.at(idx).oper, expected.at(idx).right);
        idx++;
    }
}

TEST(parsing, testEmptyHashLiteral)
{
    auto [prgrm, _] = assert_program(R"({})");
    auto* hash_lit = assert_expression<hash_literal_expression>(prgrm);
    ASSERT_TRUE(hash_lit->pairs.empty());
}
// NOLINTEND(*-magic-numbers)
