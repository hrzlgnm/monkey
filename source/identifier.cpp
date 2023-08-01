#include "identifier.hpp"

#include "environment.hpp"

identifier::identifier(token tokn, std::string val)
    : expression {tokn}
    , value {std::move(val)}
{
}
auto identifier::eval(environment_ptr env) const -> object
{
    auto val = env->get(value);
    if (val.is_nil()) {
        return make_error("identifier not found: {}", value);
    }
    return val;
}

auto identifier::string() const -> std::string
{
    return value;
}
