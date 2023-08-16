#include "if_expression.hpp"

#include <fmt/core.h>

#include "code.hpp"
#include "compiler.hpp"
#include "object.hpp"

auto if_expression::string() const -> std::string
{
    return fmt::format("if {} {} {}",
                       condition->string(),
                       consequence->string(),
                       alternative ? fmt::format("else {}", alternative->string()) : std::string());
}

inline auto is_truthy(const object& obj) -> bool
{
    return !obj.is_nil() && (!obj.is<bool>() || obj.as<bool>());
}

auto if_expression::eval(environment_ptr env) const -> object
{
    auto evaluated_condition = condition->eval(env);
    if (evaluated_condition.is<error>()) {
        return evaluated_condition;
    }
    if (is_truthy(evaluated_condition)) {
        return consequence->eval(env);
    }
    if (alternative) {
        return alternative->eval(env);
    }
    return {};
}

auto if_expression::compile(compiler& comp) const -> void
{
    condition->compile(comp);
    auto jump_not_truthy_pos = comp.emit(opcodes::jump_not_truthy, {0});
    consequence->compile(comp);
    if (comp.last_is_pop()) {
        comp.remove_last_pop();
    }
    auto jump_pos = comp.emit(opcodes::jump, {0});
    auto after_consequence = comp.cod.size();
    comp.change_operand(jump_not_truthy_pos, static_cast<int>(after_consequence));

    if (!alternative) {
        comp.emit(opcodes::null);
    } else {
        alternative->compile(comp);
        if (comp.last_is_pop()) {
            comp.remove_last_pop();
        }
    }
    auto after_alternative = comp.cod.size();
    comp.change_operand(jump_pos, static_cast<int>(after_alternative));
}
