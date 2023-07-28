#include <algorithm>
#include <cstdint>
#include <iterator>
#include <map>
#include <memory>

#include "parser.hpp"

#include <fmt/core.h>

#include "binary_expression.hpp"
#include "boolean.hpp"
#include "call_expression.hpp"
#include "function_literal.hpp"
#include "identifier.hpp"
#include "if_expression.hpp"
#include "integer_literal.hpp"
#include "program.hpp"
#include "statements.hpp"
#include "string_literal.hpp"
#include "token.hpp"
#include "token_type.hpp"
#include "unary_expression.hpp"

enum precedence
{
    lowest,
    equals,
    lessgreater,
    sum,
    product,
    prefix,
    call,
};

auto precedence_of_token(token_type type) -> int
{
    switch (type) {
        case token_type::equals:
        case token_type::not_equals:
            return equals;
        case token_type::less_than:
        case token_type::greater_than:
            return lessgreater;
        case token_type::plus:
        case token_type::minus:
            return sum;
        case token_type::slash:
        case token_type::asterisk:
            return product;
        case token_type::lparen:
            return call;
        default:
            return lowest;
    }
}

parser::parser(lexer lxr)
    : m_lxr(lxr)
{
    next_token();
    next_token();
    using enum token_type;
    register_unary(ident, [this] { return parse_identifier(); });
    register_unary(integer, [this] { return parse_integer_literal(); });
    register_unary(exclamation, [this] { return parse_unary_expression(); });
    register_unary(minus, [this] { return parse_unary_expression(); });
    register_unary(tru, [this] { return parse_boolean(); });
    register_unary(fals, [this] { return parse_boolean(); });
    register_unary(lparen, [this] { return parse_grouped_expression(); });
    register_unary(eef, [this] { return parse_if_expression(); });
    register_unary(function, [this] { return parse_function_literal(); });
    register_unary(string, [this] { return parse_string_literal(); });
    register_binary(plus, [this](expression_ptr left) { return parse_binary_expression(std::move(left)); });
    register_binary(minus, [this](expression_ptr left) { return parse_binary_expression(std::move(left)); });
    register_binary(slash, [this](expression_ptr left) { return parse_binary_expression(std::move(left)); });
    register_binary(asterisk, [this](expression_ptr left) { return parse_binary_expression(std::move(left)); });
    register_binary(equals, [this](expression_ptr left) { return parse_binary_expression(std::move(left)); });
    register_binary(not_equals, [this](expression_ptr left) { return parse_binary_expression(std::move(left)); });
    register_binary(less_than, [this](expression_ptr left) { return parse_binary_expression(std::move(left)); });
    register_binary(greater_than, [this](expression_ptr left) { return parse_binary_expression(std::move(left)); });
    register_binary(lparen, [this](expression_ptr left) { return parse_call_expression(std::move(left)); });
}

auto parser::parse_program() -> program_ptr
{
    auto prog = std::make_unique<program>();
    while (m_current_token.type != token_type::eof) {
        auto stmt = parse_statement();
        if (stmt) {
            prog->statements.push_back(std::move(stmt));
        }
        next_token();
    }
    return prog;
}

auto parser::errors() const -> const std::vector<std::string>&
{
    return m_errors;
}

auto parser::next_token() -> void
{
    m_current_token = m_peek_token;
    m_peek_token = m_lxr.next_token();
}

auto parser::parse_statement() -> statement_ptr
{
    using enum token_type;
    switch (m_current_token.type) {
        case let:
            return parse_let_statement();
        case ret:
            return parse_return_statement();
        default:
            return parse_expression_statement();
    }
}

auto parser::parse_let_statement() -> statement_ptr
{
    using enum token_type;
    auto stmt = std::make_unique<let_statement>(m_current_token);
    if (!expect_peek(ident)) {
        return {};
    }
    stmt->name = std::make_unique<identifier>(m_current_token);
    stmt->name->value = m_current_token.literal;

    if (!expect_peek(assign)) {
        return {};
    }

    next_token();
    stmt->value = parse_expression(lowest);

    if (peek_token_is(semicolon)) {
        next_token();
    }
    return stmt;
}

auto parser::parse_return_statement() -> statement_ptr
{
    using enum token_type;
    auto stmt = std::make_unique<return_statement>(m_current_token);

    next_token();
    stmt->value = parse_expression(lowest);

    if (peek_token_is(semicolon)) {
        next_token();
    }
    return stmt;
}

auto parser::parse_expression_statement() -> statement_ptr
{
    auto expr_stmt = std::make_shared<expression_statement>(m_current_token);
    expr_stmt->expr = parse_expression(lowest);
    if (peek_token_is(token_type::semicolon)) {
        next_token();
    }
    return expr_stmt;
}

auto parser::parse_expression(int precedence) -> expression_ptr
{
    auto unary = m_unary_parsers[m_current_token.type];
    if (!unary) {
        no_unary_expression_error(m_current_token.type);
        return {};
    }
    auto left_expr = unary();
    while (!peek_token_is(token_type::semicolon) && precedence < peek_precedence()) {
        auto binary = m_binary_parsers[m_peek_token.type];
        if (!binary) {
            return left_expr;
        }
        next_token();

        left_expr = binary(std::move(left_expr));
    }
    return left_expr;
}

auto parser::parse_identifier() -> identifier_ptr
{
    return std::make_unique<identifier>(m_current_token, m_current_token.literal);
}

auto parser::parse_integer_literal() -> expression_ptr
{
    auto lit = std::make_unique<integer_literal>(m_current_token);
    try {
        lit->value = std::stoll(std::string {m_current_token.literal});
    } catch (const std::out_of_range&) {
        new_error("could not parse {} as integer", m_current_token.literal);
        return {};
    }
    return lit;
}

auto parser::parse_unary_expression() -> expression_ptr
{
    auto pfx_expr = std::make_unique<unary_expression>(m_current_token);
    pfx_expr->op = m_current_token.type;

    next_token();
    pfx_expr->right = parse_expression(prefix);
    return pfx_expr;
}

auto parser::parse_boolean() -> expression_ptr
{
    return std::make_unique<boolean>(m_current_token, cur_token_is(token_type::tru));
}

auto parser::parse_grouped_expression() -> expression_ptr
{
    next_token();
    auto exp = parse_expression(lowest);
    if (!expect_peek(token_type::rparen)) {
        return {};
    }
    return exp;
}

auto parser::parse_if_expression() -> expression_ptr
{
    using enum token_type;
    auto expr = std::make_unique<if_expression>(m_current_token);
    if (!expect_peek(lparen)) {
        return {};
    }
    next_token();
    expr->condition = parse_expression(lowest);

    if (!expect_peek(rparen)) {
        return {};
    }

    if (!expect_peek(lsquirly)) {
        return {};
    }

    expr->consequence = parse_block_statement();

    if (peek_token_is(elze)) {
        next_token();
        if (!expect_peek(lsquirly)) {
            return {};
        }
        expr->alternative = parse_block_statement();
    }

    return expr;
}

auto parser::parse_function_literal() -> expression_ptr
{
    using enum token_type;
    auto literal = std::make_unique<function_literal>(m_current_token);
    if (!expect_peek(lparen)) {
        return {};
    }
    literal->parameters = parse_function_parameters();
    if (!expect_peek(lsquirly)) {
        return {};
    }
    literal->body = parse_block_statement();
    return literal;
}

auto parser::parse_function_parameters() -> std::vector<identifier_ptr>
{
    using enum token_type;
    std::vector<identifier_ptr> identifiers;
    if (peek_token_is(rparen)) {
        next_token();
        return identifiers;
    }
    next_token();
    identifiers.push_back(parse_identifier());
    while (peek_token_is(comma)) {
        next_token();
        next_token();
        identifiers.push_back(parse_identifier());
    }
    if (!expect_peek(rparen)) {
        return {};
    }
    return identifiers;
}

auto parser::parse_block_statement() -> block_statement_ptr
{
    using enum token_type;
    auto block = std::make_shared<block_statement>(m_current_token);
    next_token();
    while (!cur_token_is(rsquirly) && !cur_token_is(eof)) {
        auto stmt = parse_statement();
        if (stmt) {
            block->statements.push_back(std::move(stmt));
        }
        next_token();
    }

    return block;
}

auto parser::parse_call_expression(expression_ptr function) -> expression_ptr
{
    auto call = std::make_unique<call_expression>(m_current_token);
    call->function = std::move(function);
    call->arguments = parse_call_arguments();
    return call;
}

auto parser::parse_call_arguments() -> std::vector<expression_ptr>
{
    std::vector<expression_ptr> arguments;
    using enum token_type;
    if (peek_token_is(rparen)) {
        next_token();
        return arguments;
    }
    next_token();
    arguments.push_back(parse_expression(lowest));

    while (peek_token_is(comma)) {
        next_token();
        next_token();
        arguments.push_back(parse_expression(lowest));
    }
    if (!expect_peek(rparen)) {
        return {};
    }
    return arguments;
}

auto parser::parse_binary_expression(expression_ptr left) -> expression_ptr
{
    auto infix_expr = std::make_unique<binary_expression>(m_current_token);
    infix_expr->op = m_current_token.type;
    infix_expr->left = std::move(left);

    auto precedence = current_precedence();
    next_token();
    infix_expr->right = parse_expression(precedence);

    return infix_expr;
}

auto parser::parse_string_literal() -> expression_ptr
{
    auto lit = std::make_shared<string_literal>(m_current_token);
    lit->value = m_current_token.literal;
    return lit;
}

auto parser::expect_peek(token_type type) -> bool
{
    if (m_peek_token.type == type) {
        next_token();
        return true;
    }
    peek_error(type);
    return false;
}

auto parser::peek_error(token_type type) -> void
{
    new_error("expected next token to be {}, got {} instead", type, m_peek_token.type);
}

auto parser::register_binary(token_type type, binary_parser binary) -> void
{
    m_binary_parsers[type] = std::move(binary);
}

auto parser::register_unary(token_type type, unary_parser unary) -> void
{
    m_unary_parsers[type] = std::move(unary);
}

auto parser::cur_token_is(token_type type) const -> bool
{
    return m_current_token.type == type;
}

auto parser::peek_token_is(token_type type) const -> bool
{
    return m_peek_token.type == type;
}

auto parser::no_unary_expression_error(token_type type) -> void
{
    new_error("no prefix parse function for {} found", type);
}

auto parser::peek_precedence() const -> int
{
    return precedence_of_token(m_peek_token.type);
}

auto parser::current_precedence() const -> int
{
    return precedence_of_token(m_current_token.type);
}
