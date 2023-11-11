#include "token_type.hpp"

auto operator<<(std::ostream& ostream, token_type type) -> std::ostream&
{
    switch (type) {
        case token_type::asterisk:
            return ostream << "*";
        case token_type::back_slash:
            return ostream << "\\";
        case token_type::caret:
            return ostream << "^";
        case token_type::rsquirly:
            return ostream << "}";
        case token_type::rbracket:
            return ostream << "]";
        case token_type::rparen:
            return ostream << ")";
        case token_type::colon:
            return ostream << ":";
        case token_type::comma:
            return ostream << ",";
        case token_type::dot:
            return ostream << ".";
        case token_type::assign:
            return ostream << "=";
        case token_type::exclamation:
            return ostream << "!";
        case token_type::greater_than:
            return ostream << ">";
        case token_type::ident:
            return ostream << "identifier";
        case token_type::integer:
            return ostream << "integer";
        case token_type::string:
            return ostream << "string";
        case token_type::let:
            return ostream << "let";
        case token_type::function:
            return ostream << "fn";
        case token_type::less_than:
            return ostream << "<";
        case token_type::minus:
            return ostream << "-";
        case token_type::lsquirly:
            return ostream << "{";
        case token_type::lbracket:
            return ostream << "[";
        case token_type::lparen:
            return ostream << "(";
        case token_type::percent:
            return ostream << "%";
        case token_type::pipe:
            return ostream << "|";
        case token_type::plus:
            return ostream << "+";
        case token_type::question:
            return ostream << "?";
        case token_type::semicolon:
            return ostream << ";";
        case token_type::slash:
            return ostream << "/";
        case token_type::tilde:
            return ostream << "~";
        case token_type::illegal:
            return ostream << "illegal";
        case token_type::eof:
            return ostream << "eof";
        case token_type::tru:
            return ostream << "true";
        case token_type::fals:
            return ostream << "false";
        case token_type::eef:
            return ostream << "if";
        case token_type::elze:
            return ostream << "else";
        case token_type::ret:
            return ostream << "return";
        case token_type::equals:
            return ostream << "==";
        case token_type::not_equals:
            return ostream << "!=";
    }
}
