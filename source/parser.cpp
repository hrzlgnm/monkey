#include <algorithm>
#include <cstdint>
#include <map>
#include <memory>
#include <sstream>

#include "parser.hpp"

#include "ast.hpp"
#include "token.hpp"
#include "token_type.hpp"

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
    register_prefix(ident, [this] { return parse_identifier(); });
    register_prefix(integer, [this] { return parse_integer_literal(); });
    register_prefix(exclamation, [this] { return parse_prefix_expression(); });
    register_prefix(minus, [this] { return parse_prefix_expression(); });
    register_prefix(tru, [this] { return parse_boolean(); });
    register_prefix(fals, [this] { return parse_boolean(); });
    register_prefix(lparen, [this] { return parse_grouped_expression(); });
    register_infix(plus,
                   [this](expression_ptr left)
                   { return parse_infix_expression(std::move(left)); });
    register_infix(minus,
                   [this](expression_ptr left)
                   { return parse_infix_expression(std::move(left)); });
    register_infix(slash,
                   [this](expression_ptr left)
                   { return parse_infix_expression(std::move(left)); });
    register_infix(asterisk,
                   [this](expression_ptr left)
                   { return parse_infix_expression(std::move(left)); });
    register_infix(equals,
                   [this](expression_ptr left)
                   { return parse_infix_expression(std::move(left)); });
    register_infix(not_equals,
                   [this](expression_ptr left)
                   { return parse_infix_expression(std::move(left)); });
    register_infix(less_than,
                   [this](expression_ptr left)
                   { return parse_infix_expression(std::move(left)); });
    register_infix(greater_than,
                   [this](expression_ptr left)
                   { return parse_infix_expression(std::move(left)); });
}

auto parser::parse_program() -> std::unique_ptr<program>
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

auto parser::parse_statement() -> std::unique_ptr<statement>
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

auto parser::parse_let_statement() -> std::unique_ptr<let_statement>
{
    using enum token_type;
    auto stmt = std::make_unique<let_statement>();
    stmt->tkn = m_current_token;
    if (!expect_peek(ident)) {
        return {};
    }
    stmt->name = std::make_unique<identifier>();
    stmt->name->tkn = m_current_token;
    stmt->name->value = m_current_token.literal;

    if (!expect_peek(assign)) {
        return {};
    }

    while (!cur_token_is(semicolon)) {
        next_token();
    }
    return stmt;
}

auto parser::parse_return_statement() -> std::unique_ptr<return_statement>
{
    using enum token_type;
    auto stmt = std::make_unique<return_statement>();
    stmt->tkn = m_current_token;
    next_token();
    while (!cur_token_is(semicolon)) {
        next_token();
    }
    return stmt;
}

auto parser::parse_expression_statement()
    -> std::unique_ptr<expression_statement>
{
    using enum token_type;
    auto expr_stmt = std::make_unique<expression_statement>();
    expr_stmt->tkn = m_current_token;
    expr_stmt->expr = parse_expression(lowest);
    if (peek_token_is(token_type::semicolon)) {
        next_token();
    }
    return expr_stmt;
}
auto parser::parse_expression(int precedence) -> expression_ptr
{
    auto prefix = m_prefix_parsers[m_current_token.type];
    if (!prefix) {
        no_prefix_expression_error(m_current_token.type);
        return {};
    }
    auto left_expr = prefix();
    while (!peek_token_is(token_type::semicolon)
           && precedence < peek_precedence()) {
        auto infix = m_infix_parsers[m_peek_token.type];
        if (!infix) {
            return left_expr;
        }
        next_token();

        left_expr = infix(std::move(left_expr));
    }
    return left_expr;
}
auto parser::parse_identifier() -> expression_ptr
{
    return std::make_unique<identifier>(m_current_token,
                                        m_current_token.literal);
}

auto parser::parse_integer_literal() -> expression_ptr
{
    auto lit = std::make_unique<integer_literal>();
    lit->tkn = m_current_token;
    try {
        lit->value = std::stoll(std::string {m_current_token.literal});
    } catch (const std::out_of_range&) {
        std::stringstream strm;
        strm << "could not parse " << m_current_token.literal << " as integer";
        m_errors.push_back(strm.str());
        return {};
    }
    return lit;
}

auto parser::parse_prefix_expression() -> expression_ptr
{
    auto pfx_expr = std::make_unique<prefix_expression>();
    pfx_expr->tkn = m_current_token;
    pfx_expr->op = m_current_token.literal;

    next_token();
    pfx_expr->right = parse_expression(prefix);
    return pfx_expr;
}
auto parser::parse_boolean() -> expression_ptr
{
    return std::make_unique<boolean>(m_current_token,
                                     cur_token_is(token_type::tru));
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

auto parser::parse_infix_expression(expression_ptr left) -> expression_ptr
{
    auto infix_expr = std::make_unique<infix_expression>();
    infix_expr->tkn = m_current_token;
    infix_expr->op = m_current_token.literal;
    infix_expr->left = std::move(left);

    auto precedence = current_precedence();
    next_token();
    infix_expr->right = parse_expression(precedence);

    return infix_expr;
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
    std::ostringstream strm;
    strm << "expected next token to be " << type << ", got "
         << m_peek_token.type << " instead";
    m_errors.push_back(strm.str());
}

auto parser::register_infix(token_type type, infix_parser infix) -> void
{
    m_infix_parsers[type] = std::move(infix);
}

auto parser::register_prefix(token_type type, prefix_parser prefix) -> void
{
    m_prefix_parsers[type] = std::move(prefix);
}

auto parser::cur_token_is(token_type type) const -> bool
{
    return m_current_token.type == type;
}

auto parser::peek_token_is(token_type type) const -> bool
{
    return m_peek_token.type == type;
}

auto parser::no_prefix_expression_error(token_type type) -> void
{
    std::ostringstream strm;
    strm << "no prefix parse function for " << type << " found";
    m_errors.push_back(strm.str());
}

auto parser::peek_precedence() const -> int
{
    return precedence_of_token(m_peek_token.type);
}

auto parser::current_precedence() const -> int
{
    return precedence_of_token(m_current_token.type);
}
