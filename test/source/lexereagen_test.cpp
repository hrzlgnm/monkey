
#include <gtest/gtest.h>

#include "lib.hpp"

TEST(test, chapter1dot2)
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
        token {let, "let"},        token {identifier, "five"},
        token {assign, "="},       token {integer, "5"},
        token {semicolon, ";"},    token {let, "let"},
        token {identifier, "ten"}, token {assign, "="},
        token {integer, "10"},     token {semicolon, ";"},
        token {let, "let"},        token {identifier, "add"},
        token {assign, "="},       token {function, "fn"},
        token {lparen, "("},       token {identifier, "x"},
        token {comma, ","},        token {identifier, "y"},
        token {rparen, ")"},       token {lsquirly, "{"},
        token {identifier, "x"},   token {plus, "+"},
        token {identifier, "y"},   token {semicolon, ";"},
        token {rsquirly, "}"},     token {semicolon, ";"},
        token {let, "let"},        token {identifier, "result"},
        token {assign, "="},       token {identifier, "add"},
        token {lparen, "("},       token {identifier, "five"},
        token {comma, ","},        token {identifier, "ten"},
        token {rparen, ")"},       token {semicolon, ";"},
        token {eof, ""},
    };
    for (const auto& expected_token : expected_tokens) {
        auto token = lxr.next_token();
        ASSERT_EQ(token, expected_token);
    }
}

TEST(test, chapter1dot4)
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
        token {let, "let"},        token {identifier, "five"},
        token {assign, "="},       token {integer, "5"},
        token {semicolon, ";"},    token {let, "let"},
        token {identifier, "ten"}, token {assign, "="},
        token {integer, "10"},     token {semicolon, ";"},
        token {let, "let"},        token {identifier, "add"},
        token {assign, "="},       token {function, "fn"},
        token {lparen, "("},       token {identifier, "x"},
        token {comma, ","},        token {identifier, "y"},
        token {rparen, ")"},       token {lsquirly, "{"},
        token {identifier, "x"},   token {plus, "+"},
        token {identifier, "y"},   token {semicolon, ";"},
        token {rsquirly, "}"},     token {semicolon, ";"},
        token {let, "let"},        token {identifier, "result"},
        token {assign, "="},       token {identifier, "add"},
        token {lparen, "("},       token {identifier, "five"},
        token {comma, ","},        token {identifier, "ten"},
        token {rparen, ")"},       token {semicolon, ";"},
        token {exclamation, "!"},  token {minus, "-"},
        token {slash, "/"},        token {asterisk, "*"},
        token {integer, "5"},      token {semicolon, ";"},
        token {integer, "5"},      token {less_than, "<"},
        token {integer, "10"},     token {greater_than, ">"},
        token {integer, "5"},      token {semicolon, ";"},
        token {eef, "if"},         token {lparen, "("},
        token {integer, "5"},      token {less_than, "<"},
        token {integer, "10"},     token {rparen, ")"},
        token {lsquirly, "{"},     token {ret, "return"},
        token {tru, "true"},       token {semicolon, ";"},
        token {rsquirly, "}"},     token {elze, "else"},
        token {lsquirly, "{"},     token {ret, "return"},
        token {fals, "false"},     token {semicolon, ";"},
        token {rsquirly, "}"},     token {integer, "10"},
        token {equals, "=="},      token {integer, "10"},
        token {semicolon, ";"},    token {integer, "10"},
        token {not_equals, "!="},  token {integer, "9"},
        token {semicolon, ";"},    token {eof, ""},

    };
    for (const auto& expected_token : expected_tokens) {
        auto token = lxr.next_token();
        ASSERT_EQ(token, expected_token);
    }
}
