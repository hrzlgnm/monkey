#include <cstddef>
#include <cstdint>
#include <locale>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <system_error>

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

const auto precedences = std::map<token_type, precedence> {
    {token_type::equals, equals},
    {token_type::not_equals, equals},
    {token_type::less_than, lessgreater},
    {token_type::greater_than, lessgreater},
    {token_type::plus, sum},
    {token_type::minus, sum},
    {token_type::slash, product},
    {token_type::asterisk, product},
};

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
}

auto parser::parse_program() -> std::unique_ptr<program>
{
    auto prog = std::make_unique<program>();
    while (m_cur_token.type != token_type::eof) {
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
    m_cur_token = m_peek_token;
    m_peek_token = m_lxr.next_token();
}

auto parser::parse_statement() -> std::unique_ptr<statement>
{
    using enum token_type;
    switch (m_cur_token.type) {
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
    stmt->tkn = m_cur_token;
    if (!expect_peek(ident)) {
        return {};
    }
    stmt->name = std::make_unique<identifier>();
    stmt->name->tkn = m_cur_token;
    stmt->name->value = m_cur_token.literal;

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
    stmt->tkn = m_cur_token;
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
    expr_stmt->tkn = m_cur_token;
    expr_stmt->expr = parse_expression(lowest);
    if (peek_token_is(token_type::semicolon)) {
        next_token();
    }
    return expr_stmt;
}
auto parser::parse_expression(int precedence) -> expression_ptr
{
    (void)precedence;
    auto prefix = m_prefix_parsers[m_cur_token.type];
    if (!prefix) {
        no_prefix_expression_error(m_cur_token.type);
        return {};
    }
    return prefix();
}
auto parser::parse_identifier() -> expression_ptr
{
    return std::make_unique<identifier>(m_cur_token, m_cur_token.literal);
}

auto parser::parse_integer_literal() -> expression_ptr
{
    auto lit = std::make_unique<integer_literal>();
    lit->tkn = m_cur_token;
    try {
        lit->value = std::stoll(std::string {m_cur_token.literal});
    } catch (const std::out_of_range&) {
        std::stringstream strm;
        strm << "could not parse " << m_cur_token.literal << " as integer";
        m_errors.push_back(strm.str());
        return {};
    }
    return lit;
}

auto parser::parse_prefix_expression() -> expression_ptr
{
    auto pfx_expr = std::make_unique<prefix_expression>();
    pfx_expr->tkn = m_cur_token;
    pfx_expr->op = m_cur_token.literal;

    next_token();
    pfx_expr->right = parse_expression(prefix);
    return pfx_expr;
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
    return m_cur_token.type == type;
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
