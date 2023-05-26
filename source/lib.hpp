#pragma once

#include <ios>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>

enum class token_type
{
    ampersand,
    asterisk,
    back_slash,
    caret,
    close_squirly,
    close_bracket,
    close_paren,
    colon,
    comma,
    dot,
    equal,
    exclamation,
    greater_than,
    identifier,
    integer,
    keyword,
    less_than,
    minus,
    open_squirly,
    open_bracket,
    open_paren,
    percent,
    pipe,
    plus,
    question,
    semicolon,
    slash,
    tilde,
    unknown,
};

inline auto operator<<(std::ostream& ostream, token_type type) -> std::ostream&
{
    switch (type) {
        case token_type::ampersand:
            return ostream << "ampersand";
        case token_type::asterisk:
            return ostream << "asterisk";
        case token_type::back_slash:
            return ostream << "back_slash";
        case token_type::caret:
            return ostream << "caret";
        case token_type::close_squirly:
            return ostream << "close_squirly";
        case token_type::close_bracket:
            return ostream << "close_bracket";
        case token_type::close_paren:
            return ostream << "close_paren";
        case token_type::colon:
            return ostream << "colon";
        case token_type::comma:
            return ostream << "comma";
        case token_type::dot:
            return ostream << "dot";
        case token_type::equal:
            return ostream << "equal";
        case token_type::exclamation:
            return ostream << "exclamation";
        case token_type::greater_than:
            return ostream << "greater_than";
        case token_type::identifier:
            return ostream << "identifier";
        case token_type::integer:
            return ostream << "integer";
        case token_type::keyword:
            return ostream << "keyword";
        case token_type::less_than:
            return ostream << "less_than";
        case token_type::minus:
            return ostream << "minus";
        case token_type::open_squirly:
            return ostream << "open_squirly";
        case token_type::open_bracket:
            return ostream << "open_bracket";
        case token_type::open_paren:
            return ostream << "open_paren";
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
        case token_type::unknown:
            return ostream << "unknown";
    }
    return ostream;
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
    return ostream << "token{" << token.type << ", " << token.literal << "}";
}

class tokenizer
{
  public:
    explicit tokenizer(std::string_view contents);

    auto next_token() -> token;

  private:
    auto next_char() -> std::optional<std::string_view::value_type>;
    auto skip_whitespace() -> void;
    auto peek_char() -> std::string_view::value_type;
    auto read_identifier() -> token;
    auto read_integer() -> token;
    auto can_peek() -> bool;

    std::string_view m_contents;
    std::string_view::size_type m_position {0};
};
