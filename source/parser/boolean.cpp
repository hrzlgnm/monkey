#include "boolean.hpp"

#include "code.hpp"
#include "compiler.hpp"
#include "object.hpp"

boolean::boolean(bool val)
    : value {val}
{
}

auto boolean::eval(environment_ptr /*env*/) const -> object
{
    return object {value};
}

auto boolean::string() const -> std::string
{
    return std::string {value ? "true" : "false"};
}

auto boolean::compile(compiler& comp) const -> void
{
    comp.emit(value ? opcodes::tru : opcodes::fals);
}
