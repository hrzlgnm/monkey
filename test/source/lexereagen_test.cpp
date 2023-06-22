
#include <cstdint>
#include <memory>
#include <memory_resource>
#include <string_view>

#include "lexer.hpp"

#include <gtest/gtest.h>

#include "ast.hpp"
#include "parser.hpp"
#include "token_type.hpp"

TEST(test, chapter1dot2Lexing)
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
        token {let, "let"},      token {ident, "five"},  token {assign, "="},
        token {integer, "5"},    token {semicolon, ";"}, token {let, "let"},
        token {ident, "ten"},    token {assign, "="},    token {integer, "10"},
        token {semicolon, ";"},  token {let, "let"},     token {ident, "add"},
        token {assign, "="},     token {function, "fn"}, token {lparen, "("},
        token {ident, "x"},      token {comma, ","},     token {ident, "y"},
        token {rparen, ")"},     token {lsquirly, "{"},  token {ident, "x"},
        token {plus, "+"},       token {ident, "y"},     token {semicolon, ";"},
        token {rsquirly, "}"},   token {semicolon, ";"}, token {let, "let"},
        token {ident, "result"}, token {assign, "="},    token {ident, "add"},
        token {lparen, "("},     token {ident, "five"},  token {comma, ","},
        token {ident, "ten"},    token {rparen, ")"},    token {semicolon, ";"},
        token {eof, ""},
    };
    for (const auto& expected_token : expected_tokens) {
        auto token = lxr.next_token();
        ASSERT_EQ(token, expected_token);
    }
}

TEST(test, chapter1dot4Lexing)
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
)r"};
    auto expected_tokens = std::vector<token> {
        token {let, "let"},       token {ident, "five"},
        token {assign, "="},      token {integer, "5"},
        token {semicolon, ";"},   token {let, "let"},
        token {ident, "ten"},     token {assign, "="},
        token {integer, "10"},    token {semicolon, ";"},
        token {let, "let"},       token {ident, "add"},
        token {assign, "="},      token {function, "fn"},
        token {lparen, "("},      token {ident, "x"},
        token {comma, ","},       token {ident, "y"},
        token {rparen, ")"},      token {lsquirly, "{"},
        token {ident, "x"},       token {plus, "+"},
        token {ident, "y"},       token {semicolon, ";"},
        token {rsquirly, "}"},    token {semicolon, ";"},
        token {let, "let"},       token {ident, "result"},
        token {assign, "="},      token {ident, "add"},
        token {lparen, "("},      token {ident, "five"},
        token {comma, ","},       token {ident, "ten"},
        token {rparen, ")"},      token {semicolon, ";"},
        token {exclamation, "!"}, token {minus, "-"},
        token {slash, "/"},       token {asterisk, "*"},
        token {integer, "5"},     token {semicolon, ";"},
        token {integer, "5"},     token {less_than, "<"},
        token {integer, "10"},    token {greater_than, ">"},
        token {integer, "5"},     token {semicolon, ";"},
        token {eef, "if"},        token {lparen, "("},
        token {integer, "5"},     token {less_than, "<"},
        token {integer, "10"},    token {rparen, ")"},
        token {lsquirly, "{"},    token {ret, "return"},
        token {tru, "true"},      token {semicolon, ";"},
        token {rsquirly, "}"},    token {elze, "else"},
        token {lsquirly, "{"},    token {ret, "return"},
        token {fals, "false"},    token {semicolon, ";"},
        token {rsquirly, "}"},    token {integer, "10"},
        token {equals, "=="},     token {integer, "10"},
        token {semicolon, ";"},   token {integer, "10"},
        token {not_equals, "!="}, token {integer, "9"},
        token {semicolon, ";"},   token {eof, ""},

    };
    for (const auto& expected_token : expected_tokens) {
        auto token = lxr.next_token();
        ASSERT_EQ(token, expected_token);
    }
}

TEST(test, chapter2dod4TestLetStatement)
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
        auto* stmt = program->statements[i].get();
        auto* let_stmt = dynamic_cast<let_statement*>(stmt);
        ASSERT_TRUE(let_stmt);
        ASSERT_EQ(let_stmt->tkn.literal, "let");
        ASSERT_EQ(let_stmt->name->value, expected_identifiers[i]);
        ASSERT_EQ(let_stmt->name->tkn.literal, expected_identifiers[i]);
    }
}

TEST(test, chapter2dot4ParseError)
{
    using enum token_type;
    auto prsr = parser {lexer {
        R"r(let x = 5;
let y = 10;
let 838383;
)r"}};
    prsr.parse_program();
    auto errors = prsr.errors();
    if (errors.empty()) {
        FAIL();
    }
    for (const auto& err : errors) {
        std::cerr << "err: " << err << "\n";
    }
}

TEST(test, chapter2dot5TestReturnStatement)
{
    using enum token_type;
    auto prsr = parser {lexer {
        R"r(return 5;
return 10;
return 993322;
)r"}};
    auto program = prsr.parse_program();
    ASSERT_TRUE(program);
    ASSERT_EQ(program->statements.size(), 3);
    for (size_t i = 0; i < 3; ++i) {
        auto* stmt = program->statements[i].get();
        auto* ret_stmt = dynamic_cast<return_statement*>(stmt);
        ASSERT_TRUE(ret_stmt);
        ASSERT_EQ(ret_stmt->tkn.literal, "return");
    }
}

TEST(test, chapter2dot6TestString)
{
    using enum token_type;
    auto ltkn = token {
        .type = let,
        .literal = "let",
    };
    auto name = std::make_unique<identifier>();
    name->tkn = token {
        .type = ident,
        .literal = "myVar",
    };
    name->value = "myVar";

    auto value = std::make_unique<identifier>();
    value->tkn = token {
        .type = ident,
        .literal = "anotherVar",
    };
    value->value = "anotherVar";
    program prgrm;

    prgrm.statements.push_back(std::make_unique<let_statement>());
    dynamic_cast<let_statement*>(prgrm.statements.back().get())->tkn = ltkn;
    dynamic_cast<let_statement*>(prgrm.statements.back().get())->name =
        std::move(name);
    dynamic_cast<let_statement*>(prgrm.statements.back().get())->value =
        std::move(value);

    ASSERT_EQ(prgrm.string(), "let myVar = anotherVar;");
}

TEST(test, chapter2dot6TestIdentfierExpression)
{
    const auto* input = "foobar;";
    auto prsr = parser {lexer {input}};

    auto prgrm = prsr.parse_program();
    ASSERT_TRUE(prsr.errors().empty());
    ASSERT_EQ(prgrm->statements.size(), 1);
    auto* stmt = prgrm->statements[0].get();
    auto* expr_stmt = dynamic_cast<expression_statement*>(stmt);
    ASSERT_TRUE(expr_stmt);

    auto* expr = expr_stmt->expr.get();
    auto* ident = dynamic_cast<identifier*>(expr);
    ASSERT_TRUE(ident);

    ASSERT_EQ(ident->value, "foobar");
    ASSERT_EQ(ident->token_literal(), "foobar");
}

TEST(test, chapter2dot6TestIntegerExpression)
{
    const auto* input = "5;";
    auto prsr = parser {lexer {input}};

    auto prgrm = prsr.parse_program();
    ASSERT_TRUE(prsr.errors().empty());
    ASSERT_EQ(prgrm->statements.size(), 1);
    auto* stmt = prgrm->statements[0].get();
    auto* expr_stmt = dynamic_cast<expression_statement*>(stmt);
    ASSERT_TRUE(expr_stmt);

    auto* expr = expr_stmt->expr.get();
    auto* ident = dynamic_cast<integer_literal*>(expr);
    ASSERT_TRUE(ident);

    ASSERT_EQ(ident->value, 5);
    ASSERT_EQ(ident->token_literal(), "5");
}

TEST(test, chapter2dot6TestPrefixExpressions)
{
    struct prefix_test
    {
        std::string_view input;
        std::string op;
        int64_t integer_value;
    } prefix_tests[] = {{"!5;", "!", 5}, {"-15;", "-", 15}};

    for (const auto& prefix_test : prefix_tests) {
        auto prsr = parser {lexer {prefix_test.input}};
        auto prgrm = prsr.parse_program();
        ASSERT_TRUE(prsr.errors().empty()) << prsr.errors().at(0);
        ASSERT_EQ(prgrm->statements.size(), 1);
        auto* stmt = prgrm->statements[0].get();
        auto* expr_stmt = dynamic_cast<expression_statement*>(stmt);
        ASSERT_TRUE(expr_stmt);

        auto* expr = expr_stmt->expr.get();
        auto* prefix = dynamic_cast<prefix_expression*>(expr);
        ASSERT_TRUE(prefix);
        ASSERT_EQ(prefix_test.op, prefix->op);

        auto* ntgr_xpr = dynamic_cast<integer_literal*>(prefix->right.get());
        ASSERT_TRUE(ntgr_xpr);
        ASSERT_EQ(prefix_test.integer_value, ntgr_xpr->value);
    }
}

TEST(test, chapter2dot6TestInfixExpressions)
{
    struct infix_test
    {
        std::string_view input;
        int64_t left_value;
        std::string op;
        int64_t right_value;
    } infix_tests[] = {
        {"5 + 5;", 5, "+", 5},
        {"5 - 5;", 5, "-", 5},
        {"5 * 5;", 5, "*", 5},
        {"5 / 5;", 5, "/", 5},
        {"5 > 5;", 5, ">", 5},
        {"5 < 5;", 5, "<", 5},
        {"5 == 5;", 5, "==", 5},
        {"5 != 5;", 5, "!=", 5},
    };

    for (const auto& infix_test : infix_tests) {
        auto prsr = parser {lexer {infix_test.input}};
        auto prgrm = prsr.parse_program();
        ASSERT_TRUE(prsr.errors().empty()) << prsr.errors().at(0);
        ASSERT_EQ(prgrm->statements.size(), 1);
        auto* stmt = prgrm->statements[0].get();
        auto* expr_stmt = dynamic_cast<expression_statement*>(stmt);
        ASSERT_TRUE(expr_stmt);

        auto* expr = expr_stmt->expr.get();
        auto* infix = dynamic_cast<infix_expression*>(expr);
        ASSERT_TRUE(infix);
        ASSERT_EQ(infix_test.op, infix->op);

        auto* lft_ntgr_xpr = dynamic_cast<integer_literal*>(infix->left.get());
        ASSERT_TRUE(lft_ntgr_xpr);
        ASSERT_EQ(infix_test.left_value, lft_ntgr_xpr->value);

        auto* rght_ntgr_xpr =
            dynamic_cast<integer_literal*>(infix->right.get());
        ASSERT_TRUE(rght_ntgr_xpr);
        ASSERT_EQ(infix_test.right_value, rght_ntgr_xpr->value);
    }
}
