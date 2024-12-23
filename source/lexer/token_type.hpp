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
    ampersand,
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
    double_slash,
    shift_right,
    shift_left,
    logical_and,
    logical_or,

    // multi character tokens
    ident,
    integer,
    decimal,
    string,

    // keywords
    let,
    function,
    tru,
    fals,
    eef,
    elze,
    ret,
    hwile,
    brake,
    cont,
};

auto operator<<(std::ostream& ostream, token_type type) -> std::ostream&;

template<>
struct fmt::formatter<token_type> : ostream_formatter
{
};
