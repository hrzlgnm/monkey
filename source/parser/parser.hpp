#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include <ast/expression.hpp>
#include <ast/identifier.hpp>
#include <ast/program.hpp>
#include <lexer/lexer.hpp>
#include <lexer/token.hpp>

class parser final
{
  public:
    explicit parser(lexer lxr);
    auto parse_program() -> program_ptr;
    auto errors() const -> const std::vector<std::string>&;

  private:
    using binary_parser = std::function<expression_ptr(expression_ptr)>;
    using unary_parser = std::function<expression_ptr()>;

    auto next_token() -> void;
    auto parse_statement() -> statement_ptr;
    auto parse_let_statement() -> statement_ptr;
    auto parse_return_statement() -> statement_ptr;
    auto parse_expression_statement() -> statement_ptr;

    auto parse_expression(int precedence) -> expression_ptr;
    auto parse_identifier() const -> identifier_ptr;
    auto parse_integer_literal() -> expression_ptr;
    auto parse_unary_expression() -> expression_ptr;
    auto parse_binary_expression(expression_ptr left) -> expression_ptr;
    auto parse_boolean() -> expression_ptr;
    auto parse_grouped_expression() -> expression_ptr;
    auto parse_if_expression() -> expression_ptr;
    auto parse_function_expression() -> expression_ptr;
    auto parse_function_parameters() -> std::vector<std::string>;
    auto parse_block_statement() -> block_statement_ptr;
    auto parse_call_expression(expression_ptr function) -> expression_ptr;
    auto parse_string_literal() const -> expression_ptr;
    auto parse_array_expression() -> expression_ptr;
    auto parse_index_expression(expression_ptr left) -> expression_ptr;
    auto parse_hash_literal() -> expression_ptr;

    auto parse_expression_list(token_type end) -> std::vector<expression_ptr>;
    auto get(token_type type) -> bool;
    auto current_token_is(token_type type) const -> bool;
    auto peek_token_is(token_type type) const -> bool;
    auto peek_error(token_type type) -> void;
    auto register_binary(token_type type, binary_parser binary) -> void;
    auto register_unary(token_type type, unary_parser unary) -> void;
    auto no_unary_expression_error(token_type type) -> void;
    auto peek_precedence() const -> int;
    auto current_precedence() const -> int;

    template<typename... T>
    auto new_error(fmt::format_string<T...> fmt, T&&... args)
    {
        m_errors.push_back(fmt::format(fmt, std::forward<T>(args)...));
    }

    lexer m_lxr;
    token m_current_token {};
    token m_peek_token {};
    std::vector<std::string> m_errors {};

    std::unordered_map<token_type, unary_parser> m_unary_parsers;
    std::unordered_map<token_type, binary_parser> m_binary_parsers;
};
