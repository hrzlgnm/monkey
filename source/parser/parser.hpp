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
    auto parse_program() -> program*;
    auto errors() const -> const std::vector<std::string>&;

  private:
    using binary_parser = std::function<expression*(expression*)>;
    using unary_parser = std::function<expression*()>;

    auto next_token() -> void;
    auto parse_statement() -> statement*;
    auto parse_let_statement() -> statement*;
    auto parse_return_statement() -> statement*;
    auto parse_expression_statement() -> statement*;

    auto parse_expression(int precedence) -> expression*;
    auto parse_identifier() const -> identifier*;
    auto parse_integer_literal() -> expression*;
    auto parse_decimal_literal() -> expression*;
    auto parse_unary_expression() -> expression*;
    auto parse_binary_expression(expression* left) -> expression*;
    auto parse_boolean() -> expression*;
    auto parse_grouped_expression() -> expression*;
    auto parse_if_expression() -> expression*;
    auto parse_function_expression() -> expression*;
    auto parse_function_parameters() -> std::vector<std::string>;
    auto parse_block_statement() -> block_statement*;
    auto parse_call_expression(expression* function) -> expression*;
    auto parse_string_literal() const -> expression*;
    auto parse_array_expression() -> expression*;
    auto parse_index_expression(expression* left) -> expression*;
    auto parse_hash_literal() -> expression*;

    auto parse_expression_list(token_type end) -> std::vector<const expression*>;
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
