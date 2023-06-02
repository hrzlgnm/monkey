
#include "lexer.hpp"

#include <gtest/gtest.h>

#include "parser.hpp"

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
