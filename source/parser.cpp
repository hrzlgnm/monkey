#include <sstream>

#include "parser.hpp"

#include "token.hpp"

parser::parser(lexer lxr)
    : m_lxr(lxr)
{
    next_token();
    next_token();
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
            return {};
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

auto parser::cur_token_is(token_type type) const -> bool
{
    return m_cur_token.type == type;
}

auto parser::peek_token_is(token_type type) const -> bool
{
    return m_peek_token.type == type;
}
