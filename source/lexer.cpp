#include <algorithm>
#include <array>
#include <numeric>

#include "lexer.hpp"

#include "token_type.hpp"

using lut = std::array<token_type, std::numeric_limits<unsigned char>::max()>;

constexpr auto build_char_to_token_type_map() -> lut
{
    auto arr = lut {};
    using enum token_type;
    arr.fill(illegal);
    arr['*'] = asterisk;
    arr['^'] = caret;
    arr['}'] = rsquirly;
    arr[']'] = lbracket;
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
constexpr auto build_keywoard_to_token_type_map()
    -> std::array<std::pair<std::string_view, token_type>, keyword_count>
{
    return {std::pair {"fn", token_type::function},
            std::pair {"let", token_type::let},
            std::pair {"true", token_type::tru},
            std::pair {"false", token_type::fals},
            std::pair {"if", token_type::eef},
            std::pair {"else", token_type::elze},
            std::pair {"return", token_type::ret}};
}

constexpr auto keyword_tokens = build_keywoard_to_token_type_map();

inline auto is_letter(char chr) -> bool
{
    return std::isalpha(static_cast<unsigned char>(chr)) != 0;
}

inline auto is_digit(char chr) -> bool
{
    return std::isdigit(static_cast<unsigned char>(chr)) != 0;
}

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
        return token {eof, ""};
    }
    const auto as_uchar = static_cast<unsigned char>(m_byte);
    const auto char_token_type = char_literal_tokens[as_uchar];
    if (char_token_type != illegal) {
        const auto peek_token_type =
            char_literal_tokens[static_cast<unsigned char>(peek_char())];
        switch (char_token_type) {
            case assign:
                if (peek_token_type == assign) {
                    return read_char(), read_char(), token {equals, "=="};
                }
                break;
            case exclamation:
                if (peek_token_type == assign) {
                    return read_char(), read_char(), token {not_equals, "!="};
                }
                break;
            default:
                break;
        }
        return read_char(),
               token {
                   char_literal_tokens[as_uchar],
                   std::string_view {&m_input.at(m_position - 1), 1},
               };
    }
    if (is_letter(m_byte)) {
        return read_identifier_or_keyword();
    }
    if (is_digit(m_byte)) {
        return read_integer();
    }
    auto literal = m_byte;
    return read_char(),
           token {token_type::illegal, std::string_view {&literal, 1}};
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
    while (m_byte == ' ' || m_byte == '\t' || m_byte == '\n' || m_byte == '\r')
    {
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
    const auto* const itr =
        std::find_if(keyword_tokens.begin(),
                     keyword_tokens.end(),
                     [&identifier_or_keyword](auto keyword_pair) -> bool
                     { return keyword_pair.first == identifier_or_keyword; });
    if (itr != keyword_tokens.end()) {
        return token {itr->second, itr->first};
    }
    return token {token_type::identifier, identifier_or_keyword};
}

auto lexer::read_integer() -> token
{
    const auto position = m_position;
    while (is_digit(m_byte)) {
        read_char();
    }
    auto end = m_position;
    auto count = end - position;
    return token {token_type::integer, m_input.substr(position, count)};
}
