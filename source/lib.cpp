#include <array>
#include <cctype>
#include <cstdlib>
#include <limits>
#include <map>
#include <string_view>

#include "lib.hpp"

#include <sys/stat.h>

using lut = std::array<token_type, std::numeric_limits<unsigned char>::max()>;

constexpr auto build_char_to_token_type_map() -> lut
{
    auto arr = lut {};
    using enum token_type;
    arr.fill(unknown);
    arr['&'] = ampersand;
    arr['*'] = asterisk;
    arr['^'] = caret;
    arr['}'] = close_squirly;
    arr[']'] = close_bracket;
    arr[')'] = close_paren;
    arr[':'] = colon;
    arr[','] = comma;
    arr['='] = equal;
    arr['>'] = greater_than;
    arr['<'] = less_than;
    arr['{'] = open_squirly;
    arr['['] = open_bracket;
    arr['('] = open_paren;
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

constexpr auto char_literals = build_char_to_token_type_map();

inline auto is_letter(char chr) -> bool
{
    return std::isalpha(static_cast<unsigned char>(chr)) != 0;
}

inline auto is_digit(char chr) -> bool
{
    return std::isdigit(static_cast<unsigned char>(chr)) != 0;
}

tokenizer::tokenizer(std::string_view contents)
    : m_contents {contents}
{
}

auto tokenizer::next_token() -> token
{
    skip_whitespace();
    if (!can_peek()) {
        return token {token_type::unknown, ""};
    }
    auto as_uchar = static_cast<unsigned char>(peek_char());
    if (char_literals[as_uchar] != token_type::unknown) {
        next_char();
        return token {
            char_literals[as_uchar],
            std::string_view {&m_contents.at(m_position - 1), 1},
        };
    }
    if (is_letter(peek_char())) {
        return read_identifier();
    }
    if (is_digit(peek_char())) {
        return read_integer();
    }
    return token {token_type::unknown, ""};
}

auto tokenizer::next_char() -> std::optional<std::string_view::value_type>
{
    m_position++;
    if (can_peek()) {
        return peek_char();
    }
    return std::nullopt;
}

auto tokenizer::skip_whitespace() -> void
{
    if (!can_peek()) {
        return;
    }
    auto chur = peek_char();
    while (chur == ' ' || chur == '\t' || chur == '\n' || chur == '\r') {
        next_char();
        if (!can_peek()) {
            return;
        }
        chur = peek_char();
    }
}

auto tokenizer::peek_char() -> std::string_view::value_type
{
    return m_contents.at(m_position);
}

auto tokenizer::read_identifier() -> token
{
    const auto pos = m_position;
    while (can_peek() && is_letter(peek_char())) {
        next_char();
    }
    auto end = m_position;
    auto count = can_peek() ? end - pos : std::string_view::npos;
    return token {token_type::identifier, m_contents.substr(pos, count)};
}

auto tokenizer::read_integer() -> token
{
    const auto pos = m_position;
    while (can_peek() && is_digit(peek_char())) {
        next_char();
    }
    auto end = m_position;
    auto count = can_peek() ? end - pos : std::string_view::npos;
    return token {token_type::integer, m_contents.substr(pos, count)};
}

auto tokenizer::can_peek() -> bool
{
    return m_position + 1 <= m_contents.size();
}
