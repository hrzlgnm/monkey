#pragma once

#include <cstdint>
#include <ostream>

#include <fmt/ostream.h>

enum class token_type : std::uint8_t
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
    ident,
    integer,
    string,

    // keywords
    let,
    function,
    tru,
    fals,
    eef,
    elze,
    ret,
};

auto operator<<(std::ostream& ostream, token_type type) -> std::ostream&;

template<>
struct fmt::formatter<token_type> : ostream_formatter
{
};
