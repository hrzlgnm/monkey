#include "array_expression.hpp"

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
        result.push_back(std::make_shared<object>(evaluated));
    }
    return {result};
}
