#include <gtest/gtest.h>
#include <lexer/lexer.hpp>
#include <lexer/token_type.hpp>

TEST(lexing, testNextToken)
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
""
[1,2];
{"foo": "bar"}
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
        token {semicolon, ";"},   token {string, "foobar"},  token {string, "foo bar"}, token {string, ""},
        token {lbracket, "["},    token {integer, "1"},      token {comma, ","},        token {integer, "2"},
        token {rbracket, "]"},    token {semicolon, ";"},    token {lsquirly, "{"},     token {string, "foo"},
        token {colon, ":"},       token {string, "bar"},     token {rsquirly, "}"},     token {eof, ""},

    };
    for (const auto& expected_token : expected_tokens) {
        auto token = lxr.next_token();
        ASSERT_EQ(token, expected_token);
    }
}
