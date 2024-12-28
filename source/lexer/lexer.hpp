#pragma once
#include <string_view>

#include "token.hpp"

class lexer final
{
  public:
    explicit lexer(std::string_view input);

    auto next_token() -> token;

  private:
    auto read_char() -> void;
    auto skip_whitespace() -> void;
    auto peek_char() -> std::string_view::value_type;
    auto read_identifier_or_keyword() -> token;
    auto read_number() -> token;
    auto read_string() -> token;

    std::string_view m_input;
    std::string_view::size_type m_position {0};
    std::string_view::size_type m_read_position {0};
    std::string_view::value_type m_byte {0};
};
