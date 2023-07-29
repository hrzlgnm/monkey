#include <cmath>
#include <cstdint>
#include <deque>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

#include "lexer.hpp"

#include <gtest/gtest.h>

#include "binary_expression.hpp"
#include "boolean.hpp"
#include "call_expression.hpp"
#include "environment.hpp"
#include "function_literal.hpp"
#include "identifier.hpp"
#include "if_expression.hpp"
#include "integer_literal.hpp"
#include "object.hpp"
#include "parser.hpp"
#include "program.hpp"
#include "statements.hpp"
#include "string_literal.hpp"
#include "token.hpp"
#include "token_type.hpp"
#include "unary_expression.hpp"
#include "util.hpp"
#include "value_type.hpp"

// NOLINTBEGIN
using expected_value_type = std::variant<int64_t, std::string, bool>;

template<class>
inline constexpr bool always_false_v {false};

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

auto assert_integer_literal(const expression_ptr& expr, int64_t value) -> void
{
    auto* integer_lit = dynamic_cast<integer_literal*>(expr.get());
    ASSERT_TRUE(integer_lit) << "expected integer_literal, got " << expr.get();

    ASSERT_EQ(integer_lit->value, value);
    ASSERT_EQ(integer_lit->token_literal(), std::to_string(value));
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

auto assert_no_parse_errors(const parser& prsr)
{
    ASSERT_TRUE(prsr.errors().empty()) << "expected no errors, got: " << testing::PrintToString(prsr.errors());
}

auto assert_expression_statement(parser& prsr, const program_ptr& prgrm) -> expression_statement*
{
    assert_no_parse_errors(prsr);
    if (prgrm->statements.size() != 1) {
        throw std::invalid_argument("expected one statement, got " + std::to_string(prgrm->statements.size()));
    }
    auto* stmt = prgrm->statements[0].get();
    auto expr_stmt = dynamic_cast<expression_statement*>(stmt);
    if (!expr_stmt) {
        throw std::invalid_argument("expected expression_statement, got " + stmt->string());
    }
    return expr_stmt;
}

auto assert_let_statement(statement* stmt, const std::string& expected_identifier) -> let_statement*
{
    auto* let_stmt = dynamic_cast<let_statement*>(stmt);
    if (!let_stmt) {
        throw std::invalid_argument("expected let_statement, got " + stmt->string());
    }
    if (let_stmt->name->token_literal() != expected_identifier) {
        throw std::invalid_argument("expected identifier " + expected_identifier + ", got " + let_stmt->name->string());
    }
    return let_stmt;
}

auto assert_integer_object(const object& obj, int64_t expected) -> void
{
    auto actual = obj.as<integer_value>();
    ASSERT_EQ(actual, expected);
}

auto assert_boolean_object(const object& obj, bool expected) -> void
{
    auto actual = obj.as<bool>();
    ASSERT_EQ(actual, expected);
}

auto assert_nil_object(const object& obj) -> void
{
    ASSERT_TRUE(obj.is_nil());
}

auto assert_string_object(const object& obj, const std::string& expected) -> void
{
    auto actual = obj.as<string_value>();
    ASSERT_EQ(actual, expected);
}

TEST(lexing, lexing)
{
    using enum token_type;
    auto lxr = lexer {R"r(let five = 5;
let ten = 10;
let add = fn(x, y) {
x + y;
};
let result = add(five, ten);
)r"};
    auto expected_tokens = std::vector<token> {
        token {let, "let"},     token {ident, "five"},  token {assign, "="},  token {integer, "5"},
        token {semicolon, ";"}, token {let, "let"},     token {ident, "ten"}, token {assign, "="},
        token {integer, "10"},  token {semicolon, ";"}, token {let, "let"},   token {ident, "add"},
        token {assign, "="},    token {function, "fn"}, token {lparen, "("},  token {ident, "x"},
        token {comma, ","},     token {ident, "y"},     token {rparen, ")"},  token {lsquirly, "{"},
        token {ident, "x"},     token {plus, "+"},      token {ident, "y"},   token {semicolon, ";"},
        token {rsquirly, "}"},  token {semicolon, ";"}, token {let, "let"},   token {ident, "result"},
        token {assign, "="},    token {ident, "add"},   token {lparen, "("},  token {ident, "five"},
        token {comma, ","},     token {ident, "ten"},   token {rparen, ")"},  token {semicolon, ";"},
        token {eof, ""},
    };
    for (const auto& expected_token : expected_tokens) {
        auto token = lxr.next_token();
        ASSERT_EQ(token, expected_token);
    }
}

TEST(lexing, lexingMoreTokens)
{
    using enum token_type;
    auto lxr = lexer {R"r(let five = 5;
let ten = 10;
let add = fn(x, y) {
x + y;
};
let result = add(five, ten);
!-/*5;
5 < 10 > 5;
if (5 < 10) {
return true;
} else {
return false;
}
10 == 10;
10 != 9;
"foobar"
"foo bar"
)r"};
    auto expected_tokens = std::vector<token> {
        token {let, "let"},       token {ident, "five"},     token {assign, "="},       token {integer, "5"},
        token {semicolon, ";"},   token {let, "let"},        token {ident, "ten"},      token {assign, "="},
        token {integer, "10"},    token {semicolon, ";"},    token {let, "let"},        token {ident, "add"},
        token {assign, "="},      token {function, "fn"},    token {lparen, "("},       token {ident, "x"},
        token {comma, ","},       token {ident, "y"},        token {rparen, ")"},       token {lsquirly, "{"},
        token {ident, "x"},       token {plus, "+"},         token {ident, "y"},        token {semicolon, ";"},
        token {rsquirly, "}"},    token {semicolon, ";"},    token {let, "let"},        token {ident, "result"},
        token {assign, "="},      token {ident, "add"},      token {lparen, "("},       token {ident, "five"},
        token {comma, ","},       token {ident, "ten"},      token {rparen, ")"},       token {semicolon, ";"},
        token {exclamation, "!"}, token {minus, "-"},        token {slash, "/"},        token {asterisk, "*"},
        token {integer, "5"},     token {semicolon, ";"},    token {integer, "5"},      token {less_than, "<"},
        token {integer, "10"},    token {greater_than, ">"}, token {integer, "5"},      token {semicolon, ";"},
        token {eef, "if"},        token {lparen, "("},       token {integer, "5"},      token {less_than, "<"},
        token {integer, "10"},    token {rparen, ")"},       token {lsquirly, "{"},     token {ret, "return"},
        token {tru, "true"},      token {semicolon, ";"},    token {rsquirly, "}"},     token {elze, "else"},
        token {lsquirly, "{"},    token {ret, "return"},     token {fals, "false"},     token {semicolon, ";"},
        token {rsquirly, "}"},    token {integer, "10"},     token {equals, "=="},      token {integer, "10"},
        token {semicolon, ";"},   token {integer, "10"},     token {not_equals, "!="},  token {integer, "9"},
        token {semicolon, ";"},   token {string, "foobar"},  token {string, "foo bar"}, token {eof, ""},

    };
    for (const auto& expected_token : expected_tokens) {
        auto token = lxr.next_token();
        ASSERT_EQ(token, expected_token);
    }
}

TEST(parsing, testLetStatement)
{
    using enum token_type;
    auto prsr = parser {lexer {
        R"r(let x = 5;
let y = 10;
let foobar = 838383;
)r"}};
    auto program = prsr.parse_program();
    ASSERT_TRUE(program);
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
        auto prsr = parser {lexer {let.input}};
        auto prgrm = prsr.parse_program();
        assert_no_parse_errors(prsr);
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
    auto prsr = parser {lexer {
        R"r(return 5;
return 10;
return 993322;
)r"}};
    auto program = prsr.parse_program();
    assert_no_parse_errors(prsr);
    ASSERT_TRUE(program);
    ASSERT_EQ(program->statements.size(), 3);
    std::array expected_return_values {5, 10, 993322};
    for (size_t i = 0; i < 3; ++i) {
        auto* stmt = program->statements[i].get();
        auto* ret_stmt = dynamic_cast<return_statement*>(stmt);
        ASSERT_TRUE(ret_stmt);
        ASSERT_EQ(ret_stmt->tkn.literal, "return");
        assert_literal_expression(ret_stmt->value, expected_return_values[i]);
    }
}

TEST(parsing, testString)
{
    using enum token_type;
    auto name = std::make_unique<identifier>(token {
        .type = ident,
        .literal = "myVar",
    });
    name->value = "myVar";

    auto value = std::make_unique<identifier>(token {
        .type = ident,
        .literal = "anotherVar",
    });
    value->value = "anotherVar";

    program prgrm;

    auto let_stmt = std::make_unique<let_statement>(token {
        .type = let,
        .literal = "let",
    });
    let_stmt->name = std::move(name);
    let_stmt->value = std::move(value);
    prgrm.statements.push_back(std::move(let_stmt));

    ASSERT_EQ(prgrm.string(), "let myVar = anotherVar;");
}

TEST(parsing, testIdentfierExpression)
{
    const auto* input = "foobar;";
    auto prsr = parser {lexer {input}};
    auto prgrm = prsr.parse_program();
    auto* expr_stmt = assert_expression_statement(prsr, prgrm);

    assert_literal_expression(expr_stmt->expr, std::string {"foobar"});
}

TEST(parsing, testIntegerExpression)
{
    const auto* input = "5;";
    auto prsr = parser {lexer {input}};
    auto prgrm = prsr.parse_program();
    auto* expr_stmt = assert_expression_statement(prsr, prgrm);

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
    std::array unary_tests {
        // clang-format: off
        unary_test {"!5;", exclamation, 5},
        unary_test {"-15;", minus, 15}
        // clang-format: on
    };

    for (const auto& unary_test : unary_tests) {
        auto prsr = parser {lexer {unary_test.input}};
        auto prgrm = prsr.parse_program();
        auto* expr_stmt = assert_expression_statement(prsr, prgrm);
        auto* expr = expr_stmt->expr.get();
        auto* unary = dynamic_cast<unary_expression*>(expr);
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
        auto prsr = parser {lexer {binary_test.input}};
        auto prgrm = prsr.parse_program();
        auto* expr_stmt = assert_expression_statement(prsr, prgrm);

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

    };
    for (const auto& operator_precendce_test : operator_precedence_tests) {
        auto prsr = parser {lexer {operator_precendce_test.input}};
        auto prgrm = prsr.parse_program();
        assert_no_parse_errors(prsr);
        ASSERT_EQ(operator_precendce_test.expected, prgrm->string());
    }
}

TEST(parsing, testIfExpression)
{
    const char* input = "if (x < y) { x }";
    auto prsr = parser {lexer {input}};
    auto prgrm = prsr.parse_program();
    auto expr_stmt = assert_expression_statement(prsr, prgrm);
    auto* if_expr = dynamic_cast<if_expression*>(expr_stmt->expr.get());
    ASSERT_TRUE(if_expr);
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
    auto prsr = parser {lexer {input}};
    auto prgrm = prsr.parse_program();
    auto* expr_stmt = assert_expression_statement(prsr, prgrm);
    auto* if_expr = dynamic_cast<if_expression*>(expr_stmt->expr.get());
    ASSERT_TRUE(if_expr);

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
    auto prsr = parser {lexer {input}};
    auto prgrm = prsr.parse_program();
    auto* expr_stmt = assert_expression_statement(prsr, prgrm);
    auto* fn_literal = dynamic_cast<function_literal*>(expr_stmt->expr.get());
    ASSERT_TRUE(fn_literal);

    ASSERT_EQ(fn_literal->parameters.size(), 2);

    assert_literal_expression(fn_literal->parameters[0], "x");
    assert_literal_expression(fn_literal->parameters[1], "y");

    ASSERT_EQ(fn_literal->body->statements.size(), 1);
    auto* body_stmt = dynamic_cast<expression_statement*>(fn_literal->body->statements.at(0).get());

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
    for (const auto& parameter_test : parameter_tests) {
        auto prsr = parser {lexer {parameter_test.input}};
        auto prgrm = prsr.parse_program();
        auto* expr_stmt = assert_expression_statement(prsr, prgrm);
        auto* fn_literal = dynamic_cast<function_literal*>(expr_stmt->expr.get());
        ASSERT_TRUE(fn_literal);
        ASSERT_EQ(fn_literal->parameters.size(), parameter_test.expected.size());
        for (size_t index = 0; const auto& expected : parameter_test.expected) {
            assert_literal_expression(std::move(fn_literal->parameters[index]), expected);
            ++index;
        }
    }
}

TEST(parsing, testCallExpressionParsing)
{
    auto prsr = parser {lexer {"add(1, 2 * 3, 4 + 5);"}};
    auto prgrm = prsr.parse_program();
    auto* expr_stmt = assert_expression_statement(prsr, prgrm);
    auto* call = dynamic_cast<call_expression*>(expr_stmt->expr.get());
    ASSERT_TRUE(call);
    assert_identifier(call->function, "add");
    ASSERT_EQ(call->arguments.size(), 3);
    assert_literal_expression(call->arguments[0], 1);
    assert_binary_expression(call->arguments[1], 2, token_type::asterisk, 3);
    assert_binary_expression(call->arguments[2], 4, token_type::plus, 5);
}

TEST(parsing, testStringLiteralExpression)
{
    auto input = "\"hello world\";";
    auto prsr = parser {lexer {input}};
    auto prgrm = prsr.parse_program();
    auto* expr_stmt = assert_expression_statement(prsr, prgrm);
    auto* str = dynamic_cast<string_literal*>(expr_stmt->expr.get());
    ASSERT_TRUE(str);
    ASSERT_EQ(str->value, "hello world");
}

auto test_eval(std::string_view input) -> object
{
    auto prsr = parser {lexer {input}};
    auto prgrm = prsr.parse_program();
    auto env = std::make_shared<environment>();
    assert_no_parse_errors(prsr);
    return prgrm->eval(env);
}

TEST(eval, testEvalIntegerExpresssion)
{
    struct expression_test
    {
        std::string_view input;
        int64_t expected;
    };
    std::array expression_tests {
        expression_test {"5", 5},
        expression_test {"10", 10},
        expression_test {"-5", -5},
        expression_test {"-10", -10},
        expression_test {"5 + 5 + 5 + 5 - 10", 10},
        expression_test {"2 * 2 * 2 * 2 * 2", 32},
        expression_test {"-50 + 100 + -50", 0},
        expression_test {"5 * 2 + 10", 20},
        expression_test {"5 + 2 * 10", 25},
        expression_test {"20 + 2 * -10", 0},
        expression_test {"50 / 2 * 2 + 10", 60},
        expression_test {"2 * (5 + 10)", 30},
        expression_test {"3 * 3 * 3 + 10", 37},
        expression_test {"3 * (3 * 3) + 10", 37},
        expression_test {"(5 + 10 * 2 + 15 / 3) * 2 + -10", 50},
    };
    for (const auto& test : expression_tests) {
        const auto evaluated = test_eval(test.input);
        assert_integer_object(evaluated, test.expected);
    }
}

TEST(eval, testEvalBooleanExpresssion)
{
    struct expression_test
    {
        std::string_view input;
        bool expected;
    };
    std::array expression_tests {
        expression_test {"true", true},
        expression_test {"false", false},
        expression_test {"1 < 2", true},
        expression_test {"1 > 2", false},
        expression_test {"1 < 1", false},
        expression_test {"1 > 1", false},
        expression_test {"1 == 1", true},
        expression_test {"1 != 1", false},
        expression_test {"1 == 2", false},
        expression_test {"1 != 2", true},

    };
    for (const auto& test : expression_tests) {
        const auto evaluated = test_eval(test.input);
        assert_boolean_object(evaluated, test.expected);
    }
}

TEST(eval, testEvalStringExpression)
{
    auto evaluated = test_eval("\"Hello World!\"");
    ASSERT_TRUE(evaluated.is<string_value>());
    ASSERT_EQ(evaluated.as<string_value>(), "Hello World!");
}

TEST(eval, testEvalStringConcatenation)
{
    auto evaluated = test_eval("\"Hello\" + \" \" + \"World!\"");
    ASSERT_TRUE(evaluated.is<string_value>()) << "expected a string, got " << evaluated.type_name() << " instead";

    ASSERT_EQ(evaluated.as<string_value>(), "Hello World!");
}

TEST(eval, testBangOperator)
{
    struct expression_test
    {
        std::string_view input;
        bool expected;
    };
    std::array expression_tests {
        expression_test {"!true", false},
        expression_test {"!false", true},
        expression_test {"!false", true},
        expression_test {"!5", false},
        expression_test {"!!true", true},
        expression_test {"!!false", false},
        expression_test {"!!5", true},
    };
    for (const auto& test : expression_tests) {
        const auto evaluated = test_eval(test.input);
        assert_boolean_object(evaluated, test.expected);
    }
}

TEST(eval, testIfElseExpressions)
{
    struct expression_test
    {
        std::string_view input;
        object expected;
    };
    std::array expression_tests {
        expression_test {"if (true) { 10 }", {10}},
        expression_test {"if (false) { 10 }", {}},
        expression_test {"if (1) { 10 }", {10}},
        expression_test {"if (1 < 2) { 10 }", {10}},
        expression_test {"if (1 > 2) { 10 }", {}},
        expression_test {"if (1 > 2) { 10 } else { 20 }", {20}},
        expression_test {"if (1 < 2) { 10 } else { 20 }", {10}},
    };
    for (const auto& test : expression_tests) {
        const auto evaluated = test_eval(test.input);
        if (test.expected.is<integer_value>()) {
            assert_integer_object(evaluated, test.expected.as<integer_value>());
        } else {
            assert_nil_object(evaluated);
        }
    }
}

TEST(eval, testReturnStatemets)
{
    struct return_test
    {
        std::string_view input;
        integer_value expected;
    };
    std::array return_tests {return_test {"return 10;", 10},
                             return_test {"return 10; 9;", 10},
                             return_test {"return 2 * 5; 9;", 10},
                             return_test {"9; return 2 * 5; 9;", 10},
                             return_test {R"r(
if (10 > 1) {
    if (10 > 1) {
        return 10;
    }
    return 1;
})r",
                                          10}};
    for (const auto& test : return_tests) {
        const auto evaluated = test_eval(test.input);
        assert_integer_object(evaluated, test.expected);
    }
}

TEST(eval, testErrorHandling)
{
    struct error_test
    {
        std::string_view input;
        std::string expectedMessage;
    };
    std::array error_tests {

        error_test {
            "5 + true;",
            "type mismatch: Integer + bool",
        },
        error_test {
            "5 + true; 5;",
            "type mismatch: Integer + bool",
        },
        error_test {
            "-true",
            "unknown operator: -bool",
        },
        error_test {
            "true + false;",
            "unknown operator: bool + bool",
        },
        error_test {
            "5; true + false; 5",
            "unknown operator: bool + bool",
        },
        error_test {
            "if (10 > 1) { true + false; }",
            "unknown operator: bool + bool",
        },
        error_test {
            R"r(
if (10 > 1) {
if (10 > 1) {
return true + false;
}
return 1;
}
   )r",
            "unknown operator: bool + bool",
        },
        error_test {
            "foobar",
            "identifier not found: foobar",
        },
        error_test {
            "\"Hello\" - \"World\"",
            "unknown operator: String - String",
        }};

    for (const auto& test : error_tests) {
        const auto evaluated = test_eval(test.input);
        EXPECT_TRUE(evaluated.is<error>())
            << test.input << ": expected an error, got " << evaluated.type_name() << " instead";
        EXPECT_EQ(evaluated.as<error>().message, test.expectedMessage);
    }
}

TEST(eval, testIntegerLetStatements)
{
    struct let_test
    {
        std::string_view input;
        integer_value expected;
    };

    std::array let_tests {
        let_test {"let a = 5; a;", 5},
        let_test {"let a = 5 * 5; a;", 25},
        let_test {"let a = 5; let b = a; b;", 5},
        let_test {"let a = 5; let b = a; let c = a + b + 5; c;", 15},
    };

    for (const auto& test : let_tests) {
        assert_integer_object(test_eval(test.input), test.expected);
    }
}

TEST(eval, testFunctionObject)
{
    auto input = "fn(x) {x + 2; };";
    auto evaluated = test_eval(input);
    ASSERT_TRUE(evaluated.is<func>()) << "expected a function object, got " << std::to_string(evaluated.value)
                                      << " instead ";
}

TEST(eval, testFunctionApplication)
{
    struct func_test
    {
        std::string_view input;
        int64_t expected;
    };
    std::array func_tests {
        func_test {"let identity = fn(x) { x; }; identity(5);", 5},
        func_test {"let identity = fn(x) { return x; }; identity(5);", 5},
        func_test {"let double = fn(x) { x * 2; }; double(5);", 10},
        func_test {"let add = fn(x, y) { x + y; }; add(5, 5);", 10},
        func_test {"let add = fn(x, y) { x + y; }; add(5 + 5, add(5, 5));", 20},
        func_test {"fn(x) { x; }(5)", 5},
        func_test {"let c = fn(x) { x + 2; }; c(2 + c(4))", 10},
    };
    for (const auto test : func_tests) {
        assert_integer_object(test_eval(test.input), test.expected);
    }
}

auto test_multi_eval(std::deque<std::string>& inputs) -> object
{
    auto locals = std::make_shared<environment>();
    object result;
    while (!inputs.empty()) {
        auto prsr = parser {lexer {inputs.front()}};
        auto prgrm = prsr.parse_program();
        assert_no_parse_errors(prsr);
        result = prgrm->eval(locals);
        inputs.pop_front();
    }
    return result;
}

TEST(eval, testMultipleEvaluationsWithSameEnvAndDestroyedSources)
{
    auto input1 {"let makeGreeter = fn(greeting) { fn(name) { greeting + \" \" + name + \"!\" } };"};
    auto input2 {"let hello = makeGreeter(\"hello\");"};
    auto input3 {"hello(\"banana\");"};
    std::deque<std::string> inputs {input1, input2, input3};
    assert_string_object(test_multi_eval(inputs), "hello banana!");
}

// NOLINTEND    using statement::statement;
