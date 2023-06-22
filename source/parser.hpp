#pragma once
#include <cmath>
#include <cstdint>
#include <functional>
#include <memory>

#include "ast.hpp"
#include "lexer.hpp"
#include "token.hpp"
#include "token_type.hpp"

class parser final
{
  public:
    explicit parser(lexer lxr);
    auto parse_program() -> std::unique_ptr<program>;
    auto errors() const -> const std::vector<std::string>&;

  private:
    using infix_parser = std::function<expression_ptr(expression_ptr)>;
    using prefix_parser = std::function<expression_ptr()>;

    auto next_token() -> void;
    auto parse_statement() -> std::unique_ptr<statement>;
    auto parse_let_statement() -> std::unique_ptr<let_statement>;
    auto parse_return_statement() -> std::unique_ptr<return_statement>;
    auto parse_expression_statement() -> std::unique_ptr<expression_statement>;
    auto parse_expression(int precedence) -> expression_ptr;
    auto parse_identifier() -> expression_ptr;
    auto parse_integer_literal() -> expression_ptr;
    auto parse_prefix_expression() -> expression_ptr;
    auto parse_infix_expression(expression_ptr left) -> expression_ptr;
    auto parse_boolean() -> expression_ptr;
    auto parse_grouped_expression() -> expression_ptr;

    auto expect_peek(token_type type) -> bool;
    auto cur_token_is(token_type type) const -> bool;
    auto peek_token_is(token_type type) const -> bool;
    auto peek_error(token_type type) -> void;
    auto register_infix(token_type type, infix_parser infix) -> void;
    auto register_prefix(token_type type, prefix_parser prefix) -> void;
    auto no_prefix_expression_error(token_type type) -> void;
    auto peek_precedence() const -> int;
    auto current_precedence() const -> int;

    lexer m_lxr;
    token m_current_token {};
    token m_peek_token {};
    std::vector<std::string> m_errors {};

    std::unordered_map<token_type, prefix_parser> m_prefix_parsers;
    std::unordered_map<token_type, infix_parser> m_infix_parsers;
};
