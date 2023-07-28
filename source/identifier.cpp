#include "identifier.hpp"

#include "environment.hpp"

identifier::identifier(token tokn, std::string_view val)
    : expression {tokn}
    , value {val}
{
}
auto identifier::eval(environment_ptr env) const -> object
{
    auto val = env->get(value);
    if (!val) {
        return make_error("identifier not found: {}", value);
    }
    return val.value();
}

auto identifier::string() const -> std::string
{
    return {value.data(), value.size()};
}
