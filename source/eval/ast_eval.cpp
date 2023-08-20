#include <ast/array_expression.hpp>
#include <ast/binary_expression.hpp>
#include <ast/boolean.hpp>
#include <ast/builtin_function_expression.hpp>
#include <ast/call_expression.hpp>
#include <ast/function_expression.hpp>
#include <ast/hash_literal_expression.hpp>
#include <ast/if_expression.hpp>
#include <ast/index_expression.hpp>
#include <ast/integer_literal.hpp>
#include <ast/program.hpp>
#include <ast/string_literal.hpp>
#include <ast/unary_expression.hpp>

#include "environment.hpp"
#include "object.hpp"

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

auto eval_integer_binary_expression(token_type oper, const object& left, const object& right) -> object
{
    using enum token_type;
    auto left_int = left.as<integer_type>();
    auto right_int = right.as<integer_type>();
    switch (oper) {
        case plus:
            return {left_int + right_int};
        case minus:
            return {left_int - right_int};
        case asterisk:
            return {left_int * right_int};
        case slash:
            return {left_int / right_int};
        case less_than:
            return {left_int < right_int};
        case greater_than:
            return {left_int > right_int};
        case equals:
            return {left_int == right_int};
        case not_equals:
            return {left_int != right_int};
        default:
            return {};
    }
}

auto eval_string_binary_expression(token_type oper, const object& left, const object& right) -> object
{
    using enum token_type;
    if (oper != plus) {
        return make_error("unknown operator: {} {} {}", left.type_name(), oper, right.type_name());
    }
    const auto& left_str = left.as<string_type>();
    const auto& right_str = right.as<string_type>();
    return {left_str + right_str};
}

auto binary_expression::eval(environment_ptr env) const -> object
{
    using enum token_type;
    auto evaluated_left = left->eval(env);
    if (evaluated_left.is<error>()) {
        return evaluated_left;
    }
    auto evaluated_right = right->eval(env);
    if (evaluated_right.is<error>()) {
        return evaluated_right;
    }
    if (evaluated_left.value.index() != evaluated_right.value.index()) {
        return make_error("type mismatch: {} {} {}", evaluated_left.type_name(), op, evaluated_right.type_name());
    }
    if (evaluated_left.is<integer_type>() && evaluated_right.is<integer_type>()) {
        return eval_integer_binary_expression(op, evaluated_left, evaluated_right);
    }
    if (evaluated_left.is<string_type>() && evaluated_right.is<string_type>()) {
        return eval_string_binary_expression(op, evaluated_left, evaluated_right);
    }
    return make_error("unknown operator: {} {} {}", evaluated_left.type_name(), op, evaluated_right.type_name());
}

auto boolean::eval(environment_ptr /*env*/) const -> object
{
    return object {value};
}

auto function_expression::call(environment_ptr closure_env,
                               environment_ptr caller_env,
                               const std::vector<expression_ptr>& arguments) const -> object
{
    auto locals = std::make_shared<environment>(closure_env);
    for (auto arg_itr = arguments.begin(); const auto& parameter : parameters) {
        if (arg_itr != arguments.end()) {
            const auto& arg = *(arg_itr++);
            locals->set(parameter, arg->eval(caller_env));
        } else {
            locals->set(parameter, {});
        }
    }
    return body->eval(locals);
}

auto call_expression::eval(environment_ptr env) const -> object
{
    auto evaluated = function->eval(env);
    if (evaluated.is<error>()) {
        return evaluated;
    }
    auto [fn, closure_env] = evaluated.as<bound_function>();
    return fn->call(closure_env, env, arguments);
}

auto hash_literal_expression::eval(environment_ptr env) const -> object
{
    hash result;

    for (const auto& [key, value] : pairs) {
        auto eval_key = key->eval(env);
        if (eval_key.is<error>()) {
            return eval_key;
        }
        if (!eval_key.is_hashable()) {
            return make_error("unusable as hash key {}", eval_key.type_name());
        }
        auto eval_val = value->eval(env);
        if (eval_val.is<error>()) {
            return eval_val;
        }
        result.insert({eval_key.hash_key(), std::make_any<object>(eval_val)});
    }
    return {result};
}

auto identifier::eval(environment_ptr env) const -> object
{
    auto val = env->get(value);
    if (val.is_nil()) {
        return make_error("identifier not found: {}", value);
    }
    return val;
}

auto if_expression::eval(environment_ptr env) const -> object
{
    auto evaluated_condition = condition->eval(env);
    if (evaluated_condition.is<error>()) {
        return evaluated_condition;
    }
    if (evaluated_condition.is_truthy()) {
        return consequence->eval(env);
    }
    if (alternative) {
        return alternative->eval(env);
    }
    return {};
}

auto index_expression::eval(environment_ptr env) const -> object
{
    auto evaluated_left = left->eval(env);
    if (evaluated_left.is<error>()) {
        return evaluated_left;
    }
    auto evaluated_index = index->eval(env);
    if (evaluated_index.is<error>()) {
        return evaluated_index;
    }
    if (evaluated_left.is<array>() && evaluated_index.is<integer_type>()) {
        auto arr = evaluated_left.as<array>();
        auto index = evaluated_index.as<integer_type>();
        auto max = static_cast<int64_t>(arr.size() - 1);
        if (index < 0 || index > max) {
            return {};
        }
        return arr.at(static_cast<size_t>(index));
    }
    if (evaluated_left.is<hash>()) {
        auto hsh = evaluated_left.as<hash>();
        if (!evaluated_index.is_hashable()) {
            return make_error("unusable as hash key: {}", evaluated_index.type_name());
        }
        if (!hsh.contains(evaluated_index.hash_key())) {
            return {nil};
        }
        return unwrap(hsh.at(evaluated_index.hash_key()));
    }
    return make_error("index operator not supported: {}", evaluated_left.type_name());
}

auto integer_literal::eval(environment_ptr /*env*/) const -> object
{
    return {value};
}

auto program::eval(environment_ptr env) const -> object
{
    object result;
    for (const auto& statement : statements) {
        result = statement->eval(env);
        if (result.is_return_value) {
            return result;
        }
        if (result.is<error>()) {
            return result;
        }
    }
    return result;
}

auto let_statement::eval(environment_ptr env) const -> object
{
    auto val = value->eval(env);
    if (val.is<error>()) {
        return val;
    }
    env->set(name->value, std::move(val));
    return nil;
}

auto return_statement::eval(environment_ptr env) const -> object
{
    if (value) {
        auto evaluated = value->eval(env);
        if (evaluated.is<error>()) {
            return evaluated;
        }
        evaluated.is_return_value = true;
        return evaluated;
    }
    return nil;
}

auto expression_statement::eval(environment_ptr env) const -> object
{
    if (expr) {
        return expr->eval(env);
    }
    return nil;
}

auto block_statement::eval(environment_ptr env) const -> object
{
    object result;
    for (const auto& stmt : statements) {
        result = stmt->eval(env);
        if (result.is_return_value || result.is<error>()) {
            return result;
        }
    }
    return result;
}

auto string_literal::eval(environment_ptr /*env*/) const -> object
{
    return object {value};
}

auto unary_expression::eval(environment_ptr env) const -> object
{
    using enum token_type;
    auto evaluated_value = right->eval(env);
    if (evaluated_value.is<error>()) {
        return evaluated_value;
    }
    switch (op) {
        case minus:
            if (!evaluated_value.is<integer_type>()) {
                return make_error("unknown operator: -{}", evaluated_value.type_name());
            }
            return {-evaluated_value.as<integer_type>()};
        case exclamation:
            if (!evaluated_value.is<bool>()) {
                return {false};
            }
            return {!evaluated_value.as<bool>()};
        default:
            return make_error("unknown operator: {}{}", op, evaluated_value.type_name());
    }
}

auto builtin_function_expression::call(environment_ptr /*closure_env*/,
                                       environment_ptr caller_env,
                                       const std::vector<expression_ptr>& arguments) const -> object
{
    array args;
    std::transform(arguments.cbegin(),
                   arguments.cend(),
                   std::back_inserter(args),
                   [&caller_env](const expression_ptr& expr) { return expr->eval(caller_env); });
    return body(std::move(args));
}

const std::vector<builtin_function_expression> builtin_function_expression::builtins {
    {"len",
     {"val"},
     [](const array& arguments) -> object
     {
         if (arguments.size() != 1) {
             return make_error("wrong number of arguments to len(): expected=1, got={}", arguments.size());
         }
         return {std::visit(
             overloaded {
                 [](const string_type& str) -> object { return {static_cast<integer_type>(str.length())}; },
                 [](const array& arr) -> object { return {static_cast<integer_type>(arr.size())}; },
                 [](const auto& other) -> object
                 { return make_error("argument of type {} to len() is not supported", object {other}.type_name()); },
             },
             arguments[0].value)};
     }},
    {"puts",
     {"str"},
     [](const array& arguments) -> object
     {
         for (bool first = true; const auto& arg : arguments) {
             if (!first) {
                 fmt::print(" ");
             }
             if (arg.is<string_type>()) {
                 fmt::print("{}", arg.as<string_type>());
             } else {
                 fmt::print("{}", std::to_string(arg.value));
             }
             first = false;
         }
         fmt::print("\n");
         return {nil};
     }},
    {"first",
     {"arr"},
     [](const array& arguments) -> object
     {
         if (arguments.size() != 1) {
             return make_error("wrong number of arguments to first(): expected=1, got={}", arguments.size());
         }
         return {std::visit(
             overloaded {
                 [](const string_type& str) -> object
                 {
                     if (str.length() > 0) {
                         return {str.substr(0, 1)};
                     }
                     return {};
                 },
                 [](const array& arr) -> object
                 {
                     if (!arr.empty()) {
                         return arr.front();
                     }
                     return {};
                 },
                 [](const auto& other) -> object
                 { return make_error("argument of type {} to first() is not supported", object {other}.type_name()); },
             },
             arguments[0].value)};
     }},
    {"last",
     {"arr"},
     [](const array& arguments) -> object
     {
         if (arguments.size() != 1) {
             return make_error("wrong number of arguments to last(): expected=1, got={}", arguments.size());
         }
         return {std::visit(
             overloaded {
                 [](const string_type& str) -> object
                 {
                     if (str.length() > 1) {
                         return {str.substr(str.length() - 1, 1)};
                     }
                     return {};
                 },
                 [](const array& arr) -> object
                 {
                     if (!arr.empty()) {
                         return arr.back();
                     }
                     return {};
                 },
                 [](const auto& other) -> object
                 { return make_error("argument of type {} to last() is not supported", object {other}.type_name()); },
             },
             arguments[0].value)};
     }},
    {"rest",
     {"arr"},
     [](const array& arguments) -> object
     {
         if (arguments.size() != 1) {
             return make_error("wrong number of arguments to rest(): expected=1, got={}", arguments.size());
         }
         return {std::visit(
             overloaded {
                 [](const string_type& str) -> object
                 {
                     if (str.size() > 1) {
                         return {str.substr(1)};
                     }
                     return {};
                 },
                 [](const array& arr) -> object
                 {
                     if (arr.size() > 1) {
                         array rest;
                         std::copy(arr.begin() + 1, arr.end(), std::back_inserter(rest));
                         return {rest};
                     }
                     return {};
                 },
                 [](const auto& other) -> object
                 { return make_error("argument of type {} to rest() is not supported", object {other}.type_name()); },
             },
             arguments[0].value)};
     }},
    {"push",
     {"arr", "val"},
     [](const array& arguments) -> object
     {
         if (arguments.size() != 2) {
             return make_error("wrong number of arguments to push(): expected=2, got={}", arguments.size());
         }
         return std::visit(
             overloaded {
                 [](const array& arr, const auto& obj) -> object
                 {
                     auto copy = arr;
                     copy.push_back({obj});
                     return {copy};
                 },
                 [](const auto& other1, const auto& other2) -> object
                 {
                     return make_error("argument of type {} and {} to push() are not supported",
                                       object {other1}.type_name(),
                                       object {other2}.type_name());
                 },
             },
             arguments[0].value,
             arguments[1].value);
     }},
};

auto callable_expression::eval(environment_ptr env) const -> object
{
    return {std::make_pair(this, env)};
}
