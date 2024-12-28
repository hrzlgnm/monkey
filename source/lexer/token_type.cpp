#include <ostream>
#include <stdexcept>

#include "token_type.hpp"

auto operator<<(std::ostream& ostream, token_type type) -> std::ostream&
{
    using enum token_type;
    switch (type) {
        case asterisk:
            return ostream << "*";
        case back_slash:
            return ostream << "\\";
        case caret:
            return ostream << "^";
        case rsquirly:
            return ostream << "}";
        case rbracket:
            return ostream << "]";
        case rparen:
            return ostream << ")";
        case colon:
            return ostream << ":";
        case comma:
            return ostream << ",";
        case dot:
            return ostream << ".";
        case assign:
            return ostream << "=";
        case exclamation:
            return ostream << "!";
        case greater_than:
            return ostream << ">";
        case ident:
            return ostream << "identifier";
        case integer:
            return ostream << "integer";
        case decimal:
            return ostream << "decimal";
        case string:
            return ostream << "string";
        case let:
            return ostream << "let";
        case function:
            return ostream << "fn";
        case less_than:
            return ostream << "<";
        case minus:
            return ostream << "-";
        case lsquirly:
            return ostream << "{";
        case lbracket:
            return ostream << "[";
        case lparen:
            return ostream << "(";
        case percent:
            return ostream << "%";
        case pipe:
            return ostream << "|";
        case plus:
            return ostream << "+";
        case question:
            return ostream << "?";
        case semicolon:
            return ostream << ";";
        case slash:
            return ostream << "/";
        case tilde:
            return ostream << "~";
        case illegal:
            return ostream << "illegal";
        case eof:
            return ostream << "eof";
        case tru:
            return ostream << "true";
        case fals:
            return ostream << "false";
        case eef:
            return ostream << "if";
        case elze:
            return ostream << "else";
        case ret:
            return ostream << "return";
        case equals:
            return ostream << "==";
        case not_equals:
            return ostream << "!=";
        case double_slash:
            return ostream << "//";
        case ampersand:
            return ostream << "&";
        case shift_right:
            return ostream << ">>";
        case shift_left:
            return ostream << "<<";
        case logical_and:
            return ostream << "&&";
        case logical_or:
            return ostream << "||";
        case hwile:
            return ostream << "while";
        case brake:
            return ostream << "break";
        case cont:
            return ostream << "continue";
        case null:
            return ostream << "null";
    }
    throw std::invalid_argument("invalid token_type");
}
