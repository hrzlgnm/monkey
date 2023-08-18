#include "string_literal.hpp"

#include "code.hpp"
#include "compiler.hpp"
#include "object.hpp"
#include "util.hpp"

auto string_literal::string() const -> std::string
{
    return value;
}

auto string_literal::eval(environment_ptr /*env*/) const -> object
{
    return object {value};
}

auto string_literal::compile(compiler& comp) const -> void
{
    comp.emit(opcodes::constant, comp.add_constant({value}));
}
