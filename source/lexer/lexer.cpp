#include <algorithm>
#include <array>
#include <numeric>

#include "lexer.hpp"

#include <doctest/doctest.h>

#include "token.hpp"
#include "token_type.hpp"

using char_literal_lookup_table = std::array<token_type, std::numeric_limits<unsigned char>::max()>;

namespace
{
constexpr auto build_char_to_token_type_map() -> char_literal_lookup_table
{
    auto arr = char_literal_lookup_table {};
    using enum token_type;
    arr.fill(illegal);
    arr['*'] = asterisk;
    arr['^'] = caret;
    arr['}'] = rsquirly;
    arr[']'] = rbracket;
    arr[')'] = rparen;
    arr[':'] = colon;
    arr[','] = comma;
    arr['='] = assign;
    arr['>'] = greater_than;
    arr['<'] = less_than;
    arr['{'] = lsquirly;
    arr['['] = lbracket;
    arr['('] = lparen;
    arr[';'] = semicolon;
    arr['.'] = dot;
    arr['/'] = slash;
    arr['\\'] = back_slash;
    arr['%'] = percent;
    arr['|'] = pipe;
    arr['+'] = plus;
    arr['-'] = minus;
    arr['~'] = tilde;
    arr['!'] = exclamation;
    arr['?'] = question;
    return arr;
}

constexpr auto char_literal_tokens = build_char_to_token_type_map();
constexpr auto keyword_count = 7;
using keyword_pair = std::pair<std::string_view, token_type>;
using keyword_lookup_table = std::array<keyword_pair, keyword_count>;

constexpr auto build_keyword_to_token_type_map() -> keyword_lookup_table
{
    return {std::pair {"fn", token_type::function},
            std::pair {"let", token_type::let},
            std::pair {"true", token_type::tru},
            std::pair {"false", token_type::fals},
            std::pair {"if", token_type::eef},
            std::pair {"else", token_type::elze},
            std::pair {"return", token_type::ret}};
}

constexpr auto keyword_tokens = build_keyword_to_token_type_map();

inline auto is_letter(char chr) -> bool
{
    return std::isalpha(static_cast<unsigned char>(chr)) != 0;
}

inline auto is_digit(char chr) -> bool
{
    return std::isdigit(static_cast<unsigned char>(chr)) != 0;
}

}  // namespace

lexer::lexer(std::string_view input)
    : m_input {input}
{
    read_char();
}

auto lexer::next_token() -> token
{
    using enum token_type;
    skip_whitespace();
    if (m_byte == '\0') {
        return token {.type = eof, .literal = ""};
    }
    const auto as_uchar = static_cast<unsigned char>(m_byte);
    const auto char_token_type = char_literal_tokens.at(as_uchar);
    if (char_token_type != illegal) {
        const auto peek_token_type = char_literal_tokens.at(static_cast<unsigned char>(peek_char()));
        switch (char_token_type) {
            case assign:
                if (peek_token_type == assign) {
                    return read_char(), read_char(), token {.type = equals, .literal = "=="};
                }
                break;
            case exclamation:
                if (peek_token_type == assign) {
                    return read_char(), read_char(), token {.type = not_equals, .literal = "!="};
                }
                break;
            default:
                break;
        }
        return read_char(),
               token {
                   .type = char_literal_tokens.at(as_uchar),
                   .literal = std::string_view {&m_input.at(m_position - 1), 1},
               };
    }
    if (m_byte == '"') {
        return read_string();
    }
    if (is_letter(m_byte)) {
        return read_identifier_or_keyword();
    }
    if (is_digit(m_byte)) {
        return read_integer();
    }
    auto literal = m_byte;
    return read_char(), token {.type = token_type::illegal, .literal = std::string_view {&literal, 1}};
}

auto lexer::read_char() -> void
{
    if (m_read_position >= m_input.size()) {
        m_byte = '\0';
    } else {
        m_byte = m_input.at(m_read_position);
    }
    m_position = m_read_position;
    m_read_position++;
}

auto lexer::skip_whitespace() -> void
{
    while (m_byte == ' ' || m_byte == '\t' || m_byte == '\n' || m_byte == '\r') {
        read_char();
    }
}

auto lexer::peek_char() -> std::string_view::value_type
{
    if (m_read_position >= m_input.size()) {
        return '\0';
    }
    return m_input.at(m_read_position);
}

auto lexer::read_identifier_or_keyword() -> token
{
    const auto position = m_position;
    while (is_letter(m_byte)) {
        read_char();
    }
    auto end = m_position;
    const auto count = end - position;
    const auto identifier_or_keyword = m_input.substr(position, count);
    const auto itr =
        std::find_if(keyword_tokens.cbegin(),
                     keyword_tokens.cend(),
                     [&identifier_or_keyword](auto pair) -> bool { return pair.first == identifier_or_keyword; });
    if (itr != keyword_tokens.end()) {
        return token {.type = itr->second, .literal = itr->first};
    }
    return token {.type = token_type::ident, .literal = identifier_or_keyword};
}

auto lexer::read_integer() -> token
{
    const auto position = m_position;
    while (is_digit(m_byte)) {
        read_char();
    }
    auto end = m_position;
    auto count = end - position;
    return token {.type = token_type::integer, .literal = m_input.substr(position, count)};
}

auto lexer::read_string() -> token
{
    const auto position = m_position + 1;
    while (true) {
        read_char();
        if (m_byte == '"' || m_byte == '\0') {
            break;
        }
    }
    auto end = m_position;
    auto count = end - position;
    return read_char(), token {.type = token_type::string, .literal = m_input.substr(position, count)};
}

namespace
{
TEST_CASE("lexing")
{
    using enum token_type;
    auto lxr = lexer {
        R"(
let five = 5;
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
        )"};
    const std::array expected_tokens {
        token {.type = let, .literal = "let"},        token {.type = ident, .literal = "five"},
        token {.type = assign, .literal = "="},       token {.type = integer, .literal = "5"},
        token {.type = semicolon, .literal = ";"},    token {.type = let, .literal = "let"},
        token {.type = ident, .literal = "ten"},      token {.type = assign, .literal = "="},
        token {.type = integer, .literal = "10"},     token {.type = semicolon, .literal = ";"},
        token {.type = let, .literal = "let"},        token {.type = ident, .literal = "add"},
        token {.type = assign, .literal = "="},       token {.type = function, .literal = "fn"},
        token {.type = lparen, .literal = "("},       token {.type = ident, .literal = "x"},
        token {.type = comma, .literal = ","},        token {.type = ident, .literal = "y"},
        token {.type = rparen, .literal = ")"},       token {.type = lsquirly, .literal = "{"},
        token {.type = ident, .literal = "x"},        token {.type = plus, .literal = "+"},
        token {.type = ident, .literal = "y"},        token {.type = semicolon, .literal = ";"},
        token {.type = rsquirly, .literal = "}"},     token {.type = semicolon, .literal = ";"},
        token {.type = let, .literal = "let"},        token {.type = ident, .literal = "result"},
        token {.type = assign, .literal = "="},       token {.type = ident, .literal = "add"},
        token {.type = lparen, .literal = "("},       token {.type = ident, .literal = "five"},
        token {.type = comma, .literal = ","},        token {.type = ident, .literal = "ten"},
        token {.type = rparen, .literal = ")"},       token {.type = semicolon, .literal = ";"},
        token {.type = exclamation, .literal = "!"},  token {.type = minus, .literal = "-"},
        token {.type = slash, .literal = "/"},        token {.type = asterisk, .literal = "*"},
        token {.type = integer, .literal = "5"},      token {.type = semicolon, .literal = ";"},
        token {.type = integer, .literal = "5"},      token {.type = less_than, .literal = "<"},
        token {.type = integer, .literal = "10"},     token {.type = greater_than, .literal = ">"},
        token {.type = integer, .literal = "5"},      token {.type = semicolon, .literal = ";"},
        token {.type = eef, .literal = "if"},         token {.type = lparen, .literal = "("},
        token {.type = integer, .literal = "5"},      token {.type = less_than, .literal = "<"},
        token {.type = integer, .literal = "10"},     token {.type = rparen, .literal = ")"},
        token {.type = lsquirly, .literal = "{"},     token {.type = ret, .literal = "return"},
        token {.type = tru, .literal = "true"},       token {.type = semicolon, .literal = ";"},
        token {.type = rsquirly, .literal = "}"},     token {.type = elze, .literal = "else"},
        token {.type = lsquirly, .literal = "{"},     token {.type = ret, .literal = "return"},
        token {.type = fals, .literal = "false"},     token {.type = semicolon, .literal = ";"},
        token {.type = rsquirly, .literal = "}"},     token {.type = integer, .literal = "10"},
        token {.type = equals, .literal = "=="},      token {.type = integer, .literal = "10"},
        token {.type = semicolon, .literal = ";"},    token {.type = integer, .literal = "10"},
        token {.type = not_equals, .literal = "!="},  token {.type = integer, .literal = "9"},
        token {.type = semicolon, .literal = ";"},    token {.type = string, .literal = "foobar"},
        token {.type = string, .literal = "foo bar"}, token {.type = string, .literal = ""},
        token {.type = lbracket, .literal = "["},     token {.type = integer, .literal = "1"},
        token {.type = comma, .literal = ","},        token {.type = integer, .literal = "2"},
        token {.type = rbracket, .literal = "]"},     token {.type = semicolon, .literal = ";"},
        token {.type = lsquirly, .literal = "{"},     token {.type = string, .literal = "foo"},
        token {.type = colon, .literal = ":"},        token {.type = string, .literal = "bar"},
        token {.type = rsquirly, .literal = "}"},     token {.type = eof, .literal = ""},

    };
    for (const auto& expected_token : expected_tokens) {
        auto token = lxr.next_token();
        CHECK_EQ(token, expected_token);
    }
}
}  // namespace
