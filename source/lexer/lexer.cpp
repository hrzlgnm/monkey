#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <limits>
#include <string_view>
#include <unordered_map>
#include <utility>

#include "lexer.hpp"

#include <doctest/doctest.h>

#include "location.hpp"
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
    arr['&'] = ampersand;
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
constexpr auto keyword_count = 11;
using keyword_pair = std::pair<std::string_view, token_type>;
using keyword_lookup_table = std::array<keyword_pair, keyword_count>;

constexpr auto build_keyword_to_token_type_map() -> keyword_lookup_table
{
    return {
        std::pair {"fn", token_type::function},
        std::pair {"let", token_type::let},
        std::pair {"true", token_type::tru},
        std::pair {"false", token_type::fals},
        std::pair {"if", token_type::eef},
        std::pair {"else", token_type::elze},
        std::pair {"while", token_type::hwile},
        std::pair {"return", token_type::ret},
        std::pair {"break", token_type::brake},
        std::pair {"continue", token_type::cont},
        std::pair {"null", token_type::null},
    };
}

constexpr auto keyword_tokens = build_keyword_to_token_type_map();

using token_pair = std::pair<token_type, token_type>;

struct token_pair_hash
{
    auto operator()(const std::pair<token_type, token_type>& pair) const -> size_t
    {
        return static_cast<uint8_t>(pair.first) ^ static_cast<uint8_t>(static_cast<uint8_t>(pair.second) << 4U);
    }
};

using two_token_lookup = std::unordered_map<token_pair, token, token_pair_hash>;

auto build_two_token_lookup() -> two_token_lookup
{
    using enum token_type;
    two_token_lookup lookup;

    lookup.insert({{assign, assign},
                   {
                       .type = equals,
                       .literal = "==",
                   }});
    lookup.insert({{exclamation, assign},
                   {
                       .type = not_equals,
                       .literal = "!=",
                   }});
    lookup.insert({{exclamation, assign},
                   {
                       .type = not_equals,
                       .literal = "!=",
                   }});
    lookup.insert({{less_than, less_than},
                   {
                       .type = shift_left,
                       .literal = "<<",
                   }});
    lookup.insert({{greater_than, greater_than},
                   {
                       .type = shift_right,
                       .literal = ">>",
                   }});
    lookup.insert({{ampersand, ampersand},
                   {
                       .type = logical_and,
                       .literal = "&&",
                   }});
    lookup.insert({{pipe, pipe},
                   {
                       .type = logical_or,
                       .literal = "||",
                   }});
    lookup.insert({{slash, slash},
                   {
                       .type = double_slash,
                       .literal = "//",
                   }});
    lookup.insert({{greater_than, assign},
                   {
                       .type = greater_equal,
                       .literal = ">=",
                   }});
    lookup.insert({{less_than, assign},
                   {
                       .type = less_equal,
                       .literal = "<=",
                   }});
    return lookup;
}

inline auto is_letter(char chr) -> bool
{
    return std::isalpha(static_cast<unsigned char>(chr)) != 0 || chr == '_';
}

inline auto is_digit(char chr) -> bool
{
    return std::isdigit(static_cast<unsigned char>(chr)) != 0;
}

}  // namespace

lexer::lexer(std::string_view input, std::string_view filename)
    : m_input {input}
    , m_filename {filename}
{
    read_char();
}

auto lexer::next_token() -> token
{
    using enum token_type;
    skip_whitespace();
    const auto loc = current_loc();
    if (m_byte == '\0') {
        return token {.type = eof, .literal = "", .loc = loc};
    }
    const auto as_uchar = static_cast<unsigned char>(m_byte);
    const auto char_token_type = char_literal_tokens[as_uchar];
    const static auto two_token = build_two_token_lookup();
    if (char_token_type != illegal) {
        const auto peek_token_type = char_literal_tokens[static_cast<unsigned char>(peek_char())];
        if (const auto itr = two_token.find({char_token_type, peek_token_type}); itr != two_token.end()) {
            return read_char(), read_char(), itr->second.with_loc(loc);
        }
        return read_char(),
               token {
                   .type = char_token_type,
                   .literal = std::string_view {&m_input[m_position - 1], 1},
                   .loc = loc,
               };
    }
    if (m_byte == '"') {
        return read_string();
    }
    if (is_letter(m_byte)) {
        return read_identifier_or_keyword();
    }
    if (is_digit(m_byte)) {
        return read_number();
    }
    auto literal = m_byte;
    return read_char(),
           token {
               .type = token_type::illegal, 
               .literal = m_input.substr(m_read_position - 2, 1), 
               .loc = loc,
           ,};
}

auto lexer::read_char() -> void
{
    if (m_read_position >= m_input.size()) {
        m_byte = '\0';
    } else {
        m_byte = m_input[m_read_position];
    }
    if (m_byte == '\n') {
        m_row++;
        m_bol = m_read_position;
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
    return m_input[m_read_position];
}

auto lexer::read_identifier_or_keyword() -> token
{
    const auto loc = current_loc();
    const auto position = m_position;
    while (is_letter(m_byte)) {
        read_char();
    }
    auto end = m_position;
    const auto count = end - position;
    const auto identifier_or_keyword = m_input.substr(position, count);
    // MSVC compiler will not be happy with a const auto* const itr here
    // NOLINTBEGIN(*-qualified-auto)
    const auto itr =
        std::find_if(keyword_tokens.cbegin(),
                     keyword_tokens.cend(),
                     [&identifier_or_keyword](auto pair) -> bool { return pair.first == identifier_or_keyword; });
    if (itr != keyword_tokens.end()) {
        return token {.type = itr->second, .literal = itr->first, .loc = loc};
    }
    // NOLINTEND(*-qualified-auto)
    return token {.type = token_type::ident, .literal = identifier_or_keyword, .loc = loc};
}

auto lexer::read_number() -> token
{
    const auto loc = current_loc();
    const auto position = m_position;
    int dot_count = 0;
    while (is_digit(m_byte) || m_byte == '.') {
        if (m_byte == '.') {
            dot_count++;
        }
        read_char();
    }
    auto end = m_position;
    auto count = end - position;
    if (dot_count == 0) {
        return token {.type = token_type::integer, .literal = m_input.substr(position, count), .loc = loc};
    }
    if (dot_count == 1) {
        return token {.type = token_type::decimal, .literal = m_input.substr(position, count), .loc = loc};
    }
    return token {.type = token_type::illegal, .literal = m_input.substr(position, count), .loc = loc};
}

auto lexer::read_string() -> token
{
    const auto loc = current_loc();
    const auto position = m_position + 1;
    while (true) {
        read_char();
        if (m_byte == '"' || m_byte == '\0') {
            break;
        }
    }
    auto end = m_position;
    auto count = end - position;
    return read_char(), token {.type = token_type::string, .literal = m_input.substr(position, count), .loc = loc};
}

auto lexer::current_loc() -> location
{
    const auto column = m_row == 0 ? m_read_position - m_bol : m_read_position - m_bol - 1;
    return location {.filename = m_filename, .line = m_row + 1, .column = column};
}

namespace
{
TEST_CASE("lexing")
{
    using enum token_type;
    auto lxr = lexer {
        R"(let five = 5;
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
{"foo": "bar"};
5.5 // %
&
|
^
<<
>>
&&
||
a_b
while
break
continue
null
<=
>=
)"};
    const std::array expected_tokens {
        token {.type = let,
               .literal = "let",
               .loc {
                   .filename = "<stdin>",
                   .line = 1,
                   .column = 1,
               }},
        token {.type = ident,
               .literal = "five",
               .loc {
                   .filename = "<stdin>",
                   .line = 1,
                   .column = 5,
               }},
        token {.type = assign,
               .literal = "=",
               .loc {
                   .filename = "<stdin>",
                   .line = 1,
                   .column = 10,
               }},
        token {.type = integer,
               .literal = "5",
               .loc {
                   .filename = "<stdin>",
                   .line = 1,
                   .column = 12,
               }},
        token {.type = semicolon,
               .literal = ";",
               .loc {
                   .filename = "<stdin>",
                   .line = 1,
                   .column = 13,
               }},
        token {.type = let,
               .literal = "let",
               .loc {
                   .filename = "<stdin>",
                   .line = 2,
                   .column = 1,
               }},
        token {.type = ident,
               .literal = "ten",
               .loc {
                   .filename = "<stdin>",
                   .line = 2,
                   .column = 5,
               }},
        token {.type = assign,
               .literal = "=",
               .loc {
                   .filename = "<stdin>",
                   .line = 2,
                   .column = 9,
               }},
        token {.type = integer,
               .literal = "10",
               .loc {
                   .filename = "<stdin>",
                   .line = 2,
                   .column = 11,
               }},
        token {.type = semicolon,
               .literal = ";",
               .loc {
                   .filename = "<stdin>",
                   .line = 2,
                   .column = 13,
               }},
        token {.type = let,
               .literal = "let",
               .loc {
                   .filename = "<stdin>",
                   .line = 3,
                   .column = 1,
               }},
        token {.type = ident,
               .literal = "add",
               .loc {
                   .filename = "<stdin>",
                   .line = 3,
                   .column = 5,
               }},
        token {.type = assign,
               .literal = "=",
               .loc {
                   .filename = "<stdin>",
                   .line = 3,
                   .column = 9,
               }},
        token {.type = function,
               .literal = "fn",
               .loc {
                   .filename = "<stdin>",
                   .line = 3,
                   .column = 11,
               }},
        token {.type = lparen,
               .literal = "(",
               .loc {
                   .filename = "<stdin>",
                   .line = 3,
                   .column = 13,
               }},
        token {.type = ident,
               .literal = "x",
               .loc {
                   .filename = "<stdin>",
                   .line = 3,
                   .column = 14,
               }},
        token {.type = comma,
               .literal = ",",
               .loc {
                   .filename = "<stdin>",
                   .line = 3,
                   .column = 15,
               }},
        token {.type = ident,
               .literal = "y",
               .loc {
                   .filename = "<stdin>",
                   .line = 3,
                   .column = 17,
               }},
        token {.type = rparen,
               .literal = ")",
               .loc {
                   .filename = "<stdin>",
                   .line = 3,
                   .column = 18,
               }},
        token {.type = lsquirly,
               .literal = "{",
               .loc {
                   .filename = "<stdin>",
                   .line = 3,
                   .column = 20,
               }},
        token {.type = ident,
               .literal = "x",
               .loc {
                   .filename = "<stdin>",
                   .line = 4,
                   .column = 1,
               }},
        token {.type = plus,
               .literal = "+",
               .loc {
                   .filename = "<stdin>",
                   .line = 4,
                   .column = 3,
               }},
        token {.type = ident,
               .literal = "y",
               .loc {
                   .filename = "<stdin>",
                   .line = 4,
                   .column = 5,
               }},
        token {.type = semicolon,
               .literal = ";",
               .loc {
                   .filename = "<stdin>",
                   .line = 4,
                   .column = 6,
               }},
        token {.type = rsquirly,
               .literal = "}",
               .loc {
                   .filename = "<stdin>",
                   .line = 5,
                   .column = 1,
               }},
        token {.type = semicolon,
               .literal = ";",
               .loc {
                   .filename = "<stdin>",
                   .line = 5,
                   .column = 2,
               }},
        token {.type = let,
               .literal = "let",
               .loc {
                   .filename = "<stdin>",
                   .line = 6,
                   .column = 1,
               }},
        token {.type = ident,
               .literal = "result",
               .loc {
                   .filename = "<stdin>",
                   .line = 6,
                   .column = 5,
               }},
        token {.type = assign,
               .literal = "=",
               .loc {
                   .filename = "<stdin>",
                   .line = 6,
                   .column = 12,
               }},
        token {.type = ident,
               .literal = "add",
               .loc {
                   .filename = "<stdin>",
                   .line = 6,
                   .column = 14,
               }},
        token {.type = lparen,
               .literal = "(",
               .loc {
                   .filename = "<stdin>",
                   .line = 6,
                   .column = 17,
               }},
        token {.type = ident,
               .literal = "five",
               .loc {
                   .filename = "<stdin>",
                   .line = 6,
                   .column = 18,
               }},
        token {.type = comma,
               .literal = ",",
               .loc {
                   .filename = "<stdin>",
                   .line = 6,
                   .column = 22,
               }},
        token {.type = ident,
               .literal = "ten",
               .loc {
                   .filename = "<stdin>",
                   .line = 6,
                   .column = 24,
               }},
        token {.type = rparen,
               .literal = ")",
               .loc {
                   .filename = "<stdin>",
                   .line = 6,
                   .column = 27,
               }},
        token {.type = semicolon,
               .literal = ";",
               .loc {
                   .filename = "<stdin>",
                   .line = 6,
                   .column = 28,
               }},
        token {.type = exclamation,
               .literal = "!",
               .loc {
                   .filename = "<stdin>",
                   .line = 7,
                   .column = 1,
               }},
        token {.type = minus,
               .literal = "-",
               .loc {
                   .filename = "<stdin>",
                   .line = 7,
                   .column = 2,
               }},
        token {.type = slash,
               .literal = "/",
               .loc {
                   .filename = "<stdin>",
                   .line = 7,
                   .column = 3,
               }},
        token {.type = asterisk,
               .literal = "*",
               .loc {
                   .filename = "<stdin>",
                   .line = 7,
                   .column = 4,
               }},
        token {.type = integer,
               .literal = "5",
               .loc {
                   .filename = "<stdin>",
                   .line = 7,
                   .column = 5,
               }},
        token {.type = semicolon,
               .literal = ";",
               .loc {
                   .filename = "<stdin>",
                   .line = 7,
                   .column = 6,
               }},
        token {.type = integer,
               .literal = "5",
               .loc {
                   .filename = "<stdin>",
                   .line = 8,
                   .column = 1,
               }},
        token {.type = less_than,
               .literal = "<",
               .loc {
                   .filename = "<stdin>",
                   .line = 8,
                   .column = 3,
               }},
        token {.type = integer,
               .literal = "10",
               .loc {
                   .filename = "<stdin>",
                   .line = 8,
                   .column = 5,
               }},
        token {.type = greater_than,
               .literal = ">",
               .loc {
                   .filename = "<stdin>",
                   .line = 8,
                   .column = 8,
               }},
        token {.type = integer,
               .literal = "5",
               .loc {
                   .filename = "<stdin>",
                   .line = 8,
                   .column = 10,
               }},
        token {.type = semicolon,
               .literal = ";",
               .loc {
                   .filename = "<stdin>",
                   .line = 8,
                   .column = 11,
               }},
        token {.type = eef,
               .literal = "if",
               .loc {
                   .filename = "<stdin>",
                   .line = 9,
                   .column = 1,
               }},
        token {.type = lparen,
               .literal = "(",
               .loc {
                   .filename = "<stdin>",
                   .line = 9,
                   .column = 4,
               }},
        token {.type = integer,
               .literal = "5",
               .loc {
                   .filename = "<stdin>",
                   .line = 9,
                   .column = 5,
               }},
        token {.type = less_than,
               .literal = "<",
               .loc {
                   .filename = "<stdin>",
                   .line = 9,
                   .column = 7,
               }},
        token {.type = integer,
               .literal = "10",
               .loc {
                   .filename = "<stdin>",
                   .line = 9,
                   .column = 9,
               }},
        token {.type = rparen,
               .literal = ")",
               .loc {
                   .filename = "<stdin>",
                   .line = 9,
                   .column = 11,
               }},
        token {.type = lsquirly,
               .literal = "{",
               .loc {
                   .filename = "<stdin>",
                   .line = 9,
                   .column = 13,
               }},
        token {.type = ret,
               .literal = "return",
               .loc {
                   .filename = "<stdin>",
                   .line = 10,
                   .column = 1,
               }},
        token {.type = tru,
               .literal = "true",
               .loc {
                   .filename = "<stdin>",
                   .line = 10,
                   .column = 8,
               }},
        token {.type = semicolon,
               .literal = ";",
               .loc {
                   .filename = "<stdin>",
                   .line = 10,
                   .column = 12,
               }},
        token {.type = rsquirly,
               .literal = "}",
               .loc {
                   .filename = "<stdin>",
                   .line = 11,
                   .column = 1,
               }},
        token {.type = elze,
               .literal = "else",
               .loc {
                   .filename = "<stdin>",
                   .line = 11,
                   .column = 3,
               }},
        token {.type = lsquirly,
               .literal = "{",
               .loc {
                   .filename = "<stdin>",
                   .line = 11,
                   .column = 8,
               }},
        token {.type = ret,
               .literal = "return",
               .loc {
                   .filename = "<stdin>",
                   .line = 12,
                   .column = 1,
               }},
        token {.type = fals,
               .literal = "false",
               .loc {
                   .filename = "<stdin>",
                   .line = 12,
                   .column = 8,
               }},
        token {.type = semicolon,
               .literal = ";",
               .loc {
                   .filename = "<stdin>",
                   .line = 12,
                   .column = 13,
               }},
        token {.type = rsquirly,
               .literal = "}",
               .loc {
                   .filename = "<stdin>",
                   .line = 13,
                   .column = 1,
               }},
        token {.type = integer,
               .literal = "10",
               .loc {
                   .filename = "<stdin>",
                   .line = 14,
                   .column = 1,
               }},
        token {.type = equals,
               .literal = "==",
               .loc {
                   .filename = "<stdin>",
                   .line = 14,
                   .column = 4,
               }},
        token {.type = integer,
               .literal = "10",
               .loc {
                   .filename = "<stdin>",
                   .line = 14,
                   .column = 7,
               }},
        token {.type = semicolon,
               .literal = ";",
               .loc {
                   .filename = "<stdin>",
                   .line = 14,
                   .column = 9,
               }},
        token {.type = integer,
               .literal = "10",
               .loc {
                   .filename = "<stdin>",
                   .line = 15,
                   .column = 1,
               }},
        token {.type = not_equals,
               .literal = "!=",
               .loc {
                   .filename = "<stdin>",
                   .line = 15,
                   .column = 4,
               }},
        token {.type = integer,
               .literal = "9",
               .loc {
                   .filename = "<stdin>",
                   .line = 15,
                   .column = 7,
               }},
        token {.type = semicolon,
               .literal = ";",
               .loc {
                   .filename = "<stdin>",
                   .line = 15,
                   .column = 8,
               }},
        token {.type = string,
               .literal = "foobar",
               .loc {
                   .filename = "<stdin>",
                   .line = 16,
                   .column = 1,
               }},
        token {.type = string,
               .literal = "foo bar",
               .loc {
                   .filename = "<stdin>",
                   .line = 17,
                   .column = 1,
               }},
        token {.type = string,
               .literal = "",
               .loc {
                   .filename = "<stdin>",
                   .line = 18,
                   .column = 1,
               }},
        token {.type = lbracket,
               .literal = "[",
               .loc {
                   .filename = "<stdin>",
                   .line = 19,
                   .column = 1,
               }},
        token {.type = integer,
               .literal = "1",
               .loc {
                   .filename = "<stdin>",
                   .line = 19,
                   .column = 2,
               }},
        token {.type = comma,
               .literal = ",",
               .loc {
                   .filename = "<stdin>",
                   .line = 19,
                   .column = 3,
               }},
        token {.type = integer,
               .literal = "2",
               .loc {
                   .filename = "<stdin>",
                   .line = 19,
                   .column = 4,
               }},
        token {.type = rbracket,
               .literal = "]",
               .loc {
                   .filename = "<stdin>",
                   .line = 19,
                   .column = 5,
               }},
        token {.type = semicolon,
               .literal = ";",
               .loc {
                   .filename = "<stdin>",
                   .line = 19,
                   .column = 6,
               }},
        token {.type = lsquirly,
               .literal = "{",
               .loc {
                   .filename = "<stdin>",
                   .line = 20,
                   .column = 1,
               }},
        token {.type = string,
               .literal = "foo",
               .loc {
                   .filename = "<stdin>",
                   .line = 20,
                   .column = 2,
               }},
        token {.type = colon,
               .literal = ":",
               .loc {
                   .filename = "<stdin>",
                   .line = 20,
                   .column = 7,
               }},
        token {.type = string,
               .literal = "bar",
               .loc {
                   .filename = "<stdin>",
                   .line = 20,
                   .column = 9,
               }},
        token {.type = rsquirly,
               .literal = "}",
               .loc {
                   .filename = "<stdin>",
                   .line = 20,
                   .column = 14,
               }},
        token {.type = semicolon,
               .literal = ";",
               .loc {
                   .filename = "<stdin>",
                   .line = 20,
                   .column = 15,
               }},
        token {.type = decimal,
               .literal = "5.5",
               .loc {
                   .filename = "<stdin>",
                   .line = 21,
                   .column = 1,
               }},
        token {.type = double_slash,
               .literal = "//",
               .loc {
                   .filename = "<stdin>",
                   .line = 21,
                   .column = 5,
               }},
        token {.type = percent,
               .literal = "%",
               .loc {
                   .filename = "<stdin>",
                   .line = 21,
                   .column = 8,
               }},
        token {.type = ampersand,
               .literal = "&",
               .loc {
                   .filename = "<stdin>",
                   .line = 22,
                   .column = 1,
               }},
        token {.type = pipe,
               .literal = "|",
               .loc {
                   .filename = "<stdin>",
                   .line = 23,
                   .column = 1,
               }},
        token {.type = caret,
               .literal = "^",
               .loc {
                   .filename = "<stdin>",
                   .line = 24,
                   .column = 1,
               }},
        token {.type = shift_left,
               .literal = "<<",
               .loc {
                   .filename = "<stdin>",
                   .line = 25,
                   .column = 1,
               }},
        token {.type = shift_right,
               .literal = ">>",
               .loc {
                   .filename = "<stdin>",
                   .line = 26,
                   .column = 1,
               }},
        token {.type = logical_and,
               .literal = "&&",
               .loc {
                   .filename = "<stdin>",
                   .line = 27,
                   .column = 1,
               }},
        token {.type = logical_or,
               .literal = "||",
               .loc {
                   .filename = "<stdin>",
                   .line = 28,
                   .column = 1,
               }},
        token {.type = ident,
               .literal = "a_b",
               .loc {
                   .filename = "<stdin>",
                   .line = 29,
                   .column = 1,
               }},
        token {.type = hwile,
               .literal = "while",
               .loc {
                   .filename = "<stdin>",
                   .line = 30,
                   .column = 1,
               }},
        token {.type = brake,
               .literal = "break",
               .loc {
                   .filename = "<stdin>",
                   .line = 31,
                   .column = 1,
               }},
        token {.type = cont,
               .literal = "continue",
               .loc {
                   .filename = "<stdin>",
                   .line = 32,
                   .column = 1,
               }},
        token {.type = null,
               .literal = "null",
               .loc {
                   .filename = "<stdin>",
                   .line = 33,
                   .column = 1,
               }},
        token {.type = less_equal,
               .literal = "<=",
               .loc {
                   .filename = "<stdin>",
                   .line = 34,
                   .column = 1,
               }},
        token {.type = greater_equal,
               .literal = ">=",
               .loc {
                   .filename = "<stdin>",
                   .line = 35,
                   .column = 1,
               }},
        token {.type = eof,
               .literal = "",
               .loc {
                   .filename = "<stdin>",
                   .line = 36,
                   .column = 1,
               }},
    };

    for (const auto& expected_token : expected_tokens) {
        auto token = lxr.next_token();
        CHECK_EQ(token, expected_token);
    }
}
}  // namespace
