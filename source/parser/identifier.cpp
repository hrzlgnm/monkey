#include <stdexcept>

#include "identifier.hpp"

#include <compiler/compiler.hpp>

#include "environment.hpp"

identifier::identifier(std::string val)
    : value {std::move(val)}
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

auto identifier::compile(compiler& comp) const -> void
{
    auto symbol = comp.symbols->resolve(value);
    if (!symbol.has_value()) {
        throw std::runtime_error(fmt::format("undefined variable {}", value));
    }
    comp.emit(opcodes::get_global, {symbol.value().index});
}
