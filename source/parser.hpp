#pragma once
#include <memory>
#include <optional>

#include "ast.hpp"
#include "lexer.hpp"
#include "token.hpp"
#include "token_type.hpp"

class parser
{
  public:
    explicit parser(lexer lxr);
    auto parse_program() -> std::unique_ptr<program>;
    auto errors() const -> const std::vector<std::string>&;

  private:
    auto next_token() -> void;
    auto parse_statement() -> std::unique_ptr<statement>;
    auto parse_let_statement() -> std::unique_ptr<let_statement>;
    auto parse_return_statement() -> std::unique_ptr<return_statement>;
    auto expect_peek(token_type type) -> bool;
    auto cur_token_is(token_type type) const -> bool;
    auto peek_token_is(token_type type) const -> bool;
    auto peek_error(token_type type) -> void;

    lexer m_lxr;
    token m_cur_token {};
    token m_peek_token {};
    std::vector<std::string> m_errors {};
};
