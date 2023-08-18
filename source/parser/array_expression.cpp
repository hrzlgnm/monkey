#include "array_expression.hpp"

#include <compiler/compiler.hpp>

#include "object.hpp"
#include "util.hpp"

auto array_expression::string() const -> std::string
{
    return fmt::format("[{}]", join(elements, ", "));
}

auto array_expression::eval(environment_ptr env) const -> object
{
    array result;
    for (const auto& element : elements) {
        auto evaluated = element->eval(env);
        if (evaluated.is<error>()) {
            return evaluated;
        }
        result.push_back(std::move(evaluated));
    }
    return {result};
}

auto array_expression::compile(compiler& comp) const -> void
{
    for (const auto& element : elements) {
        element->compile(comp);
    }
    comp.emit(opcodes::array, elements.size());
}
