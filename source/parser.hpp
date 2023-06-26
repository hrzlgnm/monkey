#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "ast.hpp"
#include "lexer.hpp"
#include "token.hpp"
#include "token_type.hpp"

class parser final
{
  public:
    explicit parser(lexer lxr);
    auto parse_program() -> program_ptr;
    auto errors() const -> const std::vector<std::string>&;

  private:
    using infix_parser = std::function<expression_ptr(expression_ptr)>;
    using prefix_parser = std::function<expression_ptr()>;

    auto next_token() -> void;
    auto parse_statement() -> statement_ptr;
    auto parse_let_statement() -> statement_ptr;
    auto parse_return_statement() -> statement_ptr;
    auto parse_expression_statement() -> statement_ptr;

    auto parse_expression(int precedence) -> expression_ptr;
    auto parse_identifier() -> identifier_ptr;
    auto parse_integer_literal() -> expression_ptr;
    auto parse_prefix_expression() -> expression_ptr;
    auto parse_infix_expression(expression_ptr left) -> expression_ptr;
    auto parse_boolean() -> expression_ptr;
    auto parse_grouped_expression() -> expression_ptr;
    auto parse_if_expression() -> expression_ptr;
    auto parse_function_literal() -> expression_ptr;
    auto parse_function_parameters() -> std::vector<identifier_ptr>;
    auto parse_block_statement() -> block_statement_ptr;
    auto parse_call_expression(expression_ptr function) -> expression_ptr;
    auto parse_call_arguments() -> std::vector<expression_ptr>;

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
