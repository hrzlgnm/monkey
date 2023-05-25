
#include <gtest/gtest.h>

#include "lib.hpp"

TEST(deeznuts, deeznuts)
{
    auto lexer = lexxur {"deez nuts 1024& *^}{][)(:=><-+?;/%|~"};
    using enum token_type;
    auto le_tokens = std::vector<token> {
        token {identifier, "deez"}, token {identifier, "nuts"},
        token {integer, "1024"},    token {ampersand, "&"},
        token {asterisk, "*"},      token {caret, "^"},
        token {close_squirly, "}"}, token {open_squirly, "{"},
        token {close_bracket, "]"}, token {open_bracket, "["},
        token {close_paren, ")"},   token {open_paren, "("},
        token {colon, ":"},         token {equal, "="},
        token {greater_than, ">"},  token {less_than, "<"},
        token {minus, "-"},         token {plus, "+"},
        token {question, "?"},      token {semicolon, ";"},
        token {slash, "/"},         token {percent, "%"},
        token {pipe, "|"},          token {tilde, "~"},
        token {unknown, ""},
    };

    for (const auto& le_token : le_tokens) {
        auto token = lexer.next_token();
        ASSERT_EQ(token, le_token);
    }
}
