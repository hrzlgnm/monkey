#pragma once

#include <functional>
#include <ios>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>

enum class token_type
{
    // special tokens
    illegal,
    eof,

    // single character tokens
    assign,
    asterisk,
    back_slash,
    caret,
    colon,
    comma,
    dot,
    exclamation,
    greater_than,
    lbracket,
    less_than,
    lparen,
    minus,
    lsquirly,
    percent,
    pipe,
    plus,
    question,
    rbracket,
    rparen,
    rsquirly,
    semicolon,
    slash,
    tilde,

    // two character tokens
    equals,
    not_equals,

    // multi character tokens
    identifier,
    integer,

    // keywords
    let,
    function,
    tru,
    fals,
    eef,
    elze,
    ret,
};

inline auto operator<<(std::ostream& ostream, token_type type) -> std::ostream&
{
    switch (type) {
        case token_type::asterisk:
            return ostream << "asterisk";
        case token_type::back_slash:
            return ostream << "back_slash";
        case token_type::caret:
            return ostream << "caret";
        case token_type::rsquirly:
            return ostream << "rsquirly";
        case token_type::rbracket:
            return ostream << "rbracket";
        case token_type::rparen:
            return ostream << "rparen";
        case token_type::colon:
            return ostream << "colon";
        case token_type::comma:
            return ostream << "comma";
        case token_type::dot:
            return ostream << "dot";
        case token_type::assign:
            return ostream << "assign";
        case token_type::exclamation:
            return ostream << "exclamation";
        case token_type::greater_than:
            return ostream << "greater_than";
        case token_type::identifier:
            return ostream << "identifier";
        case token_type::integer:
            return ostream << "integer";
        case token_type::let:
            return ostream << "let";
        case token_type::function:
            return ostream << "fn";
        case token_type::less_than:
            return ostream << "less_than";
        case token_type::minus:
            return ostream << "minus";
        case token_type::lsquirly:
            return ostream << "lsquirly";
        case token_type::lbracket:
            return ostream << "lbracket";
        case token_type::lparen:
            return ostream << "lparen";
        case token_type::percent:
            return ostream << "percent";
        case token_type::pipe:
            return ostream << "pipe";
        case token_type::plus:
            return ostream << "plus";
        case token_type::question:
            return ostream << "question";
        case token_type::semicolon:
            return ostream << "semicolon";
        case token_type::slash:
            return ostream << "slash";
        case token_type::tilde:
            return ostream << "tilde";
        case token_type::illegal:
            return ostream << "illegal";
        case token_type::eof:
            return ostream << "eof";
        case token_type::tru:
            return ostream << "true";
        case token_type::fals:
            return ostream << "false";
        case token_type::eef:
            return ostream << "if";
        case token_type::elze:
            return ostream << "else";
        case token_type::ret:
            return ostream << "return";
        case token_type::equals:
            return ostream << "equals";
        case token_type::not_equals:
            return ostream << "not_equals";
    }
    __builtin_unreachable();
}

struct token
{
    token_type type;
    std::string_view literal;
    auto operator==(const token& other) const -> bool
    {
        return type == other.type && literal == other.literal;
    }
};

inline auto operator<<(std::ostream& ostream, const token& token)
    -> std::ostream&
{
    return ostream << "token{" << token.type << ", `" << token.literal << "´}";
}

class lexer
{
  public:
    explicit lexer(std::string_view input);

    auto next_token() -> token;

  private:
    auto read_char() -> void;
    auto skip_whitespace() -> void;
    auto peek_char() -> std::string_view::value_type;
    auto read_identifier_or_keyword() -> token;
    auto read_integer() -> token;

    std::string_view m_input;
    std::string_view::size_type m_position {0};
    std::string_view::size_type m_read_position {0};
    std::string_view::value_type m_byte {0};
};
