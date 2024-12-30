#include "location.hpp"

auto operator<<(std::ostream& os, const location& l) -> std::ostream&
{
    os << l.filename << ':' << l.line << ':' << l.column;
    return os;
}
