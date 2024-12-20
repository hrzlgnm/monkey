#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <iterator>
#include <sstream>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <ast/array_expression.hpp>
#include <ast/binary_expression.hpp>
#include <ast/boolean.hpp>
#include <ast/builtin_function_expression.hpp>
#include <ast/call_expression.hpp>
#include <ast/callable_expression.hpp>
#include <ast/decimal_literal.hpp>
#include <ast/expression.hpp>
#include <ast/function_expression.hpp>
#include <ast/hash_literal_expression.hpp>
#include <ast/identifier.hpp>
#include <ast/if_expression.hpp>
#include <ast/index_expression.hpp>
#include <ast/integer_literal.hpp>
#include <ast/program.hpp>
#include <ast/statements.hpp>
#include <ast/string_literal.hpp>
#include <ast/unary_expression.hpp>
#include <doctest/doctest.h>
#include <fmt/base.h>
#include <fmt/ranges.h>
#include <gc.hpp>
#include <lexer/token_type.hpp>
#include <object/object.hpp>
#include <overloaded.hpp>
#include <parser/parser.hpp>

#include "environment.hpp"

auto array_expression::eval(environment* env) const -> const object*
{
    array_object::value_type arr;
    for (const auto& element : elements) {
        const auto* evaluated = element->eval(env);
        if (evaluated->is_error()) {
            return evaluated;
        }
        arr.push_back(evaluated);
    }
    return make<array_object>(std::move(arr));
}

namespace
{
auto apply_binary_operator(token_type oper, const object* left, const object* right) -> const object*
{
    using enum token_type;
    switch (oper) {
        case plus:
            return *left + *right;
        case asterisk:
            return *left * *right;
        case minus:
            return *left - *right;
        case slash:
            return *left / *right;
        case less_than:
            return *left < *right;
        case greater_than:
            return *left > *right;
        case equals:
            return *left == *right;
        case not_equals:
            return *left != *right;
        case percent:
            return *left % *right;
        case ampersand:
            return *left & *right;
        case pipe:
            return *left | *right;
        case caret:
            return *left ^ *right;
        case shift_left:
            return *left << *right;
        case shift_right:
            return *left >> *right;
        case logical_and:
            return *left && *right;
        case double_slash:
            return floor_div(left, right);
        default:
            return {};
    }
}

}  // namespace

auto binary_expression::eval(environment* env) const -> const object*
{
    const auto* evaluated_left = left->eval(env);
    if (evaluated_left->is_error()) {
        return evaluated_left;
    }

    const auto* evaluated_right = right->eval(env);
    if (evaluated_right->is_error()) {
        return evaluated_right;
    }
    if (const auto* val = apply_binary_operator(op, evaluated_left, evaluated_right); val != nullptr) {
        return val;
    }
    if (evaluated_left->type() != evaluated_right->type()) {
        return make_error("type mismatch: {} {} {}", evaluated_left->type(), op, evaluated_right->type());
    }

    return make_error("unknown operator: {} {} {}", evaluated_left->type(), op, evaluated_right->type());
}

auto boolean::eval(environment* /*env*/) const -> const object*
{
    return native_bool_to_object(value);
}

auto function_expression::call(environment* closure_env,
                               environment* caller_env,
                               const std::vector<const expression*>& arguments) const -> const object*
{
    auto* locals = make<environment>(closure_env);
    for (auto arg_itr = arguments.begin(); const auto& parameter : parameters) {
        if (arg_itr != arguments.end()) {
            const expression* arg = *(arg_itr++);
            locals->set(parameter, arg->eval(caller_env));
        } else {
            locals->set(parameter, {});
        }
    }
    return body->eval(locals);
}

auto call_expression::eval(environment* env) const -> const object*
{
    const auto* evaluated = function->eval(env);
    if (evaluated->is_error()) {
        return evaluated;
    }
    const auto* fn = evaluated->as<function_object>();
    return fn->callable->call(fn->closure_env, env, arguments);
}

auto hash_literal_expression::eval(environment* env) const -> const object*
{
    hash_object::value_type result;

    for (const auto& [key, value] : pairs) {
        const auto* eval_key = key->eval(env);
        if (eval_key->is_error()) {
            return eval_key;
        }
        if (!eval_key->is_hashable()) {
            return make_error("unusable as hash key {}", eval_key->type());
        }
        const auto* eval_val = value->eval(env);
        if (eval_val->is_error()) {
            return eval_val;
        }
        result.insert({eval_key->as<hashable_object>()->hash_key(), eval_val});
    }
    return make<hash_object>(std::move(result));
}

auto identifier::eval(environment* env) const -> const object*
{
    const auto* val = env->get(value);
    if (val->is(object::object_type::null)) {
        return make_error("identifier not found: {}", value);
    }
    return val;
}

auto if_expression::eval(environment* env) const -> const object*
{
    const auto* evaluated_condition = condition->eval(env);
    if (evaluated_condition->is_error()) {
        return evaluated_condition;
    }
    if (evaluated_condition->is_truthy()) {
        return consequence->eval(env);
    }
    if (alternative != nullptr) {
        return alternative->eval(env);
    }
    return null_object();
}

auto index_expression::eval(environment* env) const -> const object*
{
    const auto* evaluated_left = left->eval(env);
    if (evaluated_left->is_error()) {
        return evaluated_left;
    }
    const auto* evaluated_index = index->eval(env);
    if (evaluated_index->is_error()) {
        return evaluated_index;
    }
    using enum object::object_type;
    if (evaluated_left->is(array) && evaluated_index->is(integer)) {
        const auto& arr = evaluated_left->as<array_object>()->value;
        auto index = evaluated_index->as<integer_object>()->value;
        auto max = static_cast<int64_t>(arr.size() - 1);
        if (index < 0 || index > max) {
            return null_object();
        }
        return arr[static_cast<size_t>(index)];
    }

    if (evaluated_left->is(string) && evaluated_index->is(integer)) {
        const auto& str = evaluated_left->as<string_object>()->value;
        auto index = evaluated_index->as<integer_object>()->value;
        auto max = static_cast<int64_t>(str.size() - 1);
        if (index < 0 || index > max) {
            return null_object();
        }
        return make<string_object>(str.substr(static_cast<size_t>(index), 1));
    }

    if (evaluated_left->is(hash)) {
        const auto& hsh = evaluated_left->as<hash_object>()->value;
        if (!evaluated_index->is_hashable()) {
            return make_error("unusable as hash key: {}", evaluated_index->type());
        }
        const auto hash_key = evaluated_index->as<hashable_object>()->hash_key();
        if (!hsh.contains(hash_key)) {
            return null_object();
        }
        return hsh.at(hash_key);
    }
    return make_error("index operator not supported: {}", evaluated_left->type());
}

auto integer_literal::eval(environment* /*env*/) const -> object*
{
    return make<integer_object>(value);
}

auto decimal_literal::eval(environment* /*env*/) const -> object*
{
    return make<decimal_object>(value);
}

auto program::eval(environment* env) const -> const object*
{
    const object* result = nullptr;
    for (const auto* statement : statements) {
        result = statement->eval(env);
        if (result->is_return_value()) {
            return result->as<return_value_object>()->return_value;
        }
        if (result->is_error()) {
            return result;
        }
    }
    return result;
}

auto let_statement::eval(environment* env) const -> const object*
{
    const auto* val = value->eval(env);
    if (val->is_error()) {
        return val;
    }
    env->set(name->value, val);
    return null_object();
}

auto return_statement::eval(environment* env) const -> const object*
{
    if (value != nullptr) {
        const auto* evaluated = value->eval(env);
        if (evaluated->is_error()) {
            return evaluated;
        }
        return make<return_value_object>(evaluated);
    }
    return null_object();
}

auto expression_statement::eval(environment* env) const -> const object*
{
    if (expr != nullptr) {
        return expr->eval(env);
    }
    return null_object();
}

auto block_statement::eval(environment* env) const -> const object*
{
    const object* result = nullptr;
    for (const auto& stmt : statements) {
        result = stmt->eval(env);
        if (result->is_return_value() || result->is_error()) {
            return result;
        }
    }
    return result;
}

auto string_literal::eval(environment* /*env*/) const -> const object*
{
    return make<string_object>(value);
}

auto unary_expression::eval(environment* env) const -> const object*
{
    using enum token_type;
    const auto* evaluated_value = right->eval(env);
    if (evaluated_value->is_error()) {
        return evaluated_value;
    }
    switch (op) {
        case minus:
            if (evaluated_value->is(object::object_type::integer)) {
                return make<integer_object>(-evaluated_value->as<integer_object>()->value);
            } else if (evaluated_value->is(object::object_type::decimal)) {
                return make<decimal_object>(-evaluated_value->as<decimal_object>()->value);
            }
            return make_error("unknown operator: -{}", evaluated_value->type());
        case exclamation:
            return native_bool_to_object(!evaluated_value->is_truthy());
        default:
            return make_error("unknown operator: {}{}", op, evaluated_value->type());
    }
}

auto builtin_function_expression::call(environment* /*closure_env*/,
                                       environment* caller_env,
                                       const std::vector<const expression*>& arguments) const -> const object*
{
    array_object::value_type args;
    std::transform(arguments.cbegin(),
                   arguments.cend(),
                   std::back_inserter(args),
                   [caller_env](const expression* expr) { return expr->eval(caller_env); });
    return body(std::move(args));
}

namespace
{

const builtin_function_expression builtin_len {
    "len",
    {"val"},
    [](const array_object::value_type& arguments) -> const object*
    {
        if (arguments.size() != 1) {
            return make_error("wrong number of arguments to len(): expected=1, got={}", arguments.size());
        }
        const auto& maybe_string_or_array_or_hash = arguments[0];
        using enum object::object_type;
        if (maybe_string_or_array_or_hash->is(string)) {
            const auto& str = maybe_string_or_array_or_hash->as<string_object>()->value;
            return make<integer_object>(static_cast<int64_t>(str.size()));
        }
        if (maybe_string_or_array_or_hash->is(array)) {
            const auto& arr = maybe_string_or_array_or_hash->as<array_object>()->value;

            return make<integer_object>(static_cast<int64_t>(arr.size()));
        }
        if (maybe_string_or_array_or_hash->is(hash)) {
            const auto& hsh = maybe_string_or_array_or_hash->as<hash_object>()->value;

            return make<integer_object>(static_cast<int64_t>(hsh.size()));
        }
        return make_error("argument of type {} to len() is not supported", maybe_string_or_array_or_hash->type());
    }};

const builtin_function_expression builtin_puts {"puts",
                                                {"val"},
                                                [](const array_object::value_type& arguments) -> const object*
                                                {
                                                    using enum object::object_type;
                                                    for (bool first = true; const auto& arg : arguments) {
                                                        if (!first) {
                                                            fmt::print(" ");
                                                        }
                                                        if (arg->is(string)) {
                                                            fmt::print("{}", arg->as<string_object>()->value);
                                                        } else {
                                                            fmt::print("{}", arg->inspect());
                                                        }
                                                        first = false;
                                                    }
                                                    fmt::print("\n");
                                                    return null_object();
                                                }};

const builtin_function_expression builtin_first {
    "first",
    {"arr"},
    [](const array_object::value_type& arguments) -> const object*
    {
        if (arguments.size() != 1) {
            return make_error("wrong number of arguments to first(): expected=1, got={}", arguments.size());
        }
        const auto& maybe_string_or_array = arguments.at(0);
        using enum object::object_type;
        if (maybe_string_or_array->is(string)) {
            const auto& str = maybe_string_or_array->as<string_object>()->value;
            if (!str.empty()) {
                return make<string_object>(str.substr(0, 1));
            }
            return null_object();
        }
        if (maybe_string_or_array->is(array)) {
            const auto& arr = maybe_string_or_array->as<array_object>()->value;
            if (!arr.empty()) {
                return arr.front();
            }
            return null_object();
        }
        return make_error("argument of type {} to first() is not supported", maybe_string_or_array->type());
    }};

const builtin_function_expression builtin_last {
    "last",
    {"arr"},
    [](const array_object::value_type& arguments) -> const object*
    {
        if (arguments.size() != 1) {
            return make_error("wrong number of arguments to last(): expected=1, got={}", arguments.size());
        }
        const auto& maybe_string_or_array = arguments[0];
        using enum object::object_type;
        if (maybe_string_or_array->is(string)) {
            const auto& str = maybe_string_or_array->as<string_object>()->value;
            if (!str.empty()) {
                return make<string_object>(str.substr(str.length() - 1, 1));
            }
            return null_object();
        }
        if (maybe_string_or_array->is(array)) {
            const auto& arr = maybe_string_or_array->as<array_object>()->value;
            if (!arr.empty()) {
                return arr.back();
            }
            return null_object();
        }
        return make_error("argument of type {} to last() is not supported", maybe_string_or_array->type());
    }};

const builtin_function_expression builtin_rest {
    "rest",
    {"arr"},
    [](const array_object::value_type& arguments) -> const object*
    {
        if (arguments.size() != 1) {
            return make_error("wrong number of arguments to rest(): expected=1, got={}", arguments.size());
        }
        const auto& maybe_string_or_array = arguments.at(0);
        using enum object::object_type;
        if (maybe_string_or_array->is(string)) {
            const auto& str = maybe_string_or_array->as<string_object>()->value;
            if (str.size() > 1) {
                return make<string_object>(str.substr(1));
            }
            return null_object();
        }
        if (maybe_string_or_array->is(array)) {
            const auto& arr = maybe_string_or_array->as<array_object>()->value;
            if (arr.size() > 1) {
                array_object::value_type rest;
                std::copy(arr.cbegin() + 1, arr.cend(), std::back_inserter(rest));
                return make<array_object>(std::move(rest));
            }
            return null_object();
        }
        return make_error("argument of type {} to rest() is not supported", maybe_string_or_array->type());
    }};

const builtin_function_expression builtin_push {
    "push",
    {"arr", "val"},
    [](const array_object::value_type& arguments) -> const object*
    {
        if (arguments.size() != 2) {
            return make_error("wrong number of arguments to push(): expected=2, got={}", arguments.size());
        }
        const auto& lhs = arguments[0];
        const auto& rhs = arguments[1];
        using enum object::object_type;
        if (lhs->is(array)) {
            auto copy = lhs->as<array_object>()->value;
            copy.push_back(rhs);
            return make<array_object>(std::move(copy));
        }
        if (lhs->is(string) && rhs->is(string)) {
            auto copy = lhs->as<string_object>()->value;
            copy.append(rhs->as<string_object>()->value);
            return make<string_object>(std::move(copy));
        }
        return make_error("argument of type {} and {} to push() are not supported", lhs->type(), rhs->type());
    }};

const builtin_function_expression builtin_type {
    "type",
    {"val"},
    [](const array_object::value_type& arguments) -> const object*
    {
        if (arguments.size() != 1) {
            return make_error("wrong number of arguments to type(): expected=1, got={}", arguments.size());
        }
        const auto& val = arguments[0];
        std::ostringstream strm;
        strm << val->type();
        return make<string_object>(strm.str());
    }};
}  // namespace

const std::vector<const builtin_function_expression*> builtin_function_expression::builtins {
    &builtin_len, &builtin_puts, &builtin_first, &builtin_last, &builtin_rest, &builtin_push, &builtin_type};

auto callable_expression::eval(environment* env) const -> const object*
{
    return make<function_object>(this, env);
}

namespace
{
struct error
{
    std::string message;
};

using array = std::vector<std::variant<std::string, int64_t>>;
using null_type = std::monostate;
const null_type null_value {};

// NOLINTBEGIN(*)
auto require_eq(const object* obj, const int64_t expected, std::string_view input) -> void
{
    INFO(input, " expected: integer ", " with: ", expected, " got: ", obj->type(), " with: ", obj->inspect());
    REQUIRE(obj->is(object::object_type::integer));
    const auto& actual = obj->as<integer_object>()->value;
    REQUIRE(actual == expected);
}

auto require_eq(const object* obj, const bool expected, std::string_view input) -> void
{
    INFO(input, " expected: boolean ", " with: ", expected, " got: ", obj->type(), " with: ", obj->inspect());
    REQUIRE(obj->is(object::object_type::boolean));
    const auto& actual = obj->as<boolean_object>()->value;
    REQUIRE(actual == expected);
}

auto require_eq(const object* obj, const std::string& expected, std::string_view input) -> void
{
    INFO(input, " expected: string with: ", expected, " got: ", obj->type(), " with: ", obj->inspect());
    REQUIRE(obj->is(object::object_type::string));
    const auto& actual = obj->as<string_object>()->value;
    REQUIRE(actual == expected);
}

auto require_eq(const object* obj, double expected, std::string_view input) -> void
{
    INFO(input, " expected: string with: ", expected, " got: ", obj->type(), " with: ", obj->inspect());
    REQUIRE(obj->is(object::object_type::decimal));
    const auto& actual = obj->as<decimal_object>()->value;
    constexpr auto epsilon = 1e-9;
    REQUIRE(std::fabs(actual - expected) < epsilon);
}

auto require_array_eq(const object* obj, const array& expected, std::string_view input) -> void
{
    INFO(input, " expected: array with: ", expected.size(), "elements got: ", obj->type(), " with: ", obj->inspect());
    REQUIRE(obj->is(object::object_type::array));
    const auto& actual = obj->as<array_object>()->value;
    REQUIRE(actual.size() == expected.size());
    for (auto idx = 0UL; const auto& expected_elem : expected) {
        std::visit(
            overloaded {
                [&](const int64_t exp) { REQUIRE_EQ(exp, actual[idx]->as<integer_object>()->value); },
                [&](const std::string& exp) { REQUIRE_EQ(exp, actual[idx]->as<string_object>()->value); },
            },
            expected_elem);
        ++idx;
    }
}

auto require_error_eq(const object* obj, const std::string& expected, std::string_view input) -> void
{
    INFO(input, " expected: error with message: ", expected, " got: ", obj->type(), " with: ", obj->inspect());
    REQUIRE(obj->is(object::object_type::error));
    const auto& actual = obj->as<error_object>()->value;
    REQUIRE(actual == expected);
}

auto check_no_parse_errors(const parser& prsr) -> bool
{
    INFO("expected no errors, got:\n", fmt::format("{}", fmt::join(prsr.errors(), "\n")));
    CHECK(prsr.errors().empty());
    return prsr.errors().empty();
}

using parsed_program = std::pair<program*, parser>;

auto check_program(std::string_view input) -> parsed_program
{
    auto prsr = parser {lexer {input}};
    auto prgrm = prsr.parse_program();
    INFO("while parsing: `", input, "`");
    CHECK(check_no_parse_errors(prsr));
    return {prgrm, std::move(prsr)};
}

auto run(std::string_view input) -> const object*
{
    auto [prgrm, _] = check_program(input);
    environment env;
    for (const auto& builtin : builtin_function_expression::builtins) {
        env.set(builtin->name, make<builtin_object>(builtin));
    }
    auto result = prgrm->eval(&env);
    return result;
}

auto run_multi(std::deque<std::string>& inputs) -> const object*
{
    environment env;
    const object* result = nullptr;
    while (!inputs.empty()) {
        auto [prgrm, _] = check_program(inputs.front());
        result = prgrm->eval(&env);
        inputs.pop_front();
    }
    return result;
}

TEST_SUITE_BEGIN("eval");

TEST_CASE("integerExpression")
{
    struct et
    {
        std::string_view input;
        int64_t expected;
    };

    std::array tests {
        et {"5", 5},
        et {"10", 10},
        et {"-5", -5},
        et {"-10", -10},
        et {"5 + 5 + 5 + 5 - 10", 10},
        et {"2 * 2 * 2 * 2 * 2", 32},
        et {"-50 + 100 + -50", 0},
        et {"5 * 2 + 10", 20},
        et {"5 + 2 * 10", 25},
        et {"20 + 2 * -10", 0},
        et {"50 / 2 * 2 + 10", 60},
        et {"2 * (5 + 10)", 30},
        et {"3 * 3 * 3 + 10", 37},
        et {"3 * (3 * 3) + 10", 37},
        et {"(5 + 10 * 2 + 15 / 3) * 2 + -10", 50},
        et {"5 // 2", 2},
        et {"5 // -2", -3},
        et {"-2 // 2", -1},
        et {"-2 // -2", 1},
        et {"3 % 2", 1},
        et {"-1 % 100", 99},
        et {"2 % 5", 2},
        et {"2 & 5", 0},
        et {"3 & 5", 1},
        et {"1 & 1", 1},
        et {"1 & true", 1},
        et {"1 & false", 0},
        et {"2 | 5", 7},
        et {"3 | 5", 7},
        et {"1 | 1", 1},
        et {"1 | true", 1},
        et {"1 | false", 1},
        et {"0 | false", 0},
        et {"2 ^ 5", 7},
        et {"3 ^ 5", 6},
        et {"1 ^ 1", 0},
        et {"1 ^ true", 0},
        et {"1 ^ false", 1},
        et {"0 ^ false", 0},
        et {"2 << 5", 64},
        et {"3 << 5", 96},
        et {"1 << 1", 2},
        et {"1 << true", 2},
        et {"1 << false", 1},
        et {"0 << false", 0},
        et {"2 >> 5", 0},
        et {"3 >> 5", 0},
        et {"4 >> 1", 2},
        et {"1 >> true", 0},
        et {"1 >> false", 1},
        et {"0 >> false", 0},
        et {"false << false", 0},
        et {"true << false", 1},
        et {"true << true", 2},
        et {"false >> false", 0},
        et {"true >> false", 1},
        et {"true >> true", 0},
    };
    for (const auto& [input, expected] : tests) {
        const auto evaluated = run(input);
        require_eq(evaluated, expected, input);
    }
}

TEST_CASE("decimalExpression")
{
    struct et
    {
        std::string_view input;
        double expected;
    };

    std::array tests {
        et {"5.0", 5.0},
        et {"10.0", 10.0},
        et {"-5.0", -5.0},
        et {"-10.0", -10.0},
        et {"5.0 + 5.0 + 5.0 + 5.0 - 10.0", 10.0},
        et {"2.0 * 2.0 * 2.0 * 2.0 * 2.0", 32.0},
        et {"-50.0 + 100.0 + -50.0", 0.0},
        et {"5.0 * 2.0 + 10.0", 20.0},
        et {"5.0 + 2.0 * 10.0", 25.0},
        et {"20.0 + 2.0 * -10.0", 0.0},
        et {"50.0/ 2.0 * 2.0 + 10.0", 60.0},
        et {"2.0 * (5.0 + 10.0)", 30.0},
        et {"3.0 * 3.0 * 3.0 + 10.0", 37.0},
        et {"3.0 * (3.0 * 3.0) + 10.0", 37.0},
        et {"(5.0 + 10.0 * 2.0 + 15.0 / 3.0) * 2.0 + -10.8", 49.2},
        et {"5 + 10.2", 15.2},
        et {"10.2 + 5", 15.2},
        et {"5 - 10.1", -5.1},
        et {"10.1 - 5", 5.1},
        et {"5 * 10.0", 50.0},
        et {"10.0 * 5", 50.0},
        et {"10 / 5.0", 2.0},
        et {"10.0 // 2", 5.0},
        et {"-5.0 // -2", 2.0},
        et {"5.0 // -2", -3.0},
        et {"10.0 % 5", 0.0},
        et {"10.0 % 5.0", 0.0},
        et {"5.2 % 2.6", 0.0},
        et {"2.6 % 5.2", 2.6},
        et {"-2.6 % 5.2", 2.6},
        et {"10.0 / 5", 2.0},
    };
    for (const auto& [input, expected] : tests) {
        const auto evaluated = run(input);
        require_eq(evaluated, expected, input);
    }
}

TEST_CASE("booleanExpression")
{
    struct et
    {
        std::string_view input;
        bool expected;
    };

    std::array tests {
        et {"true", true},
        et {"false", false},
        et {"1 < 2", true},
        et {"1 > 2", false},
        et {"1 < 1", false},
        et {"1 > 1", false},
        et {"1.1 > 1.1", false},
        et {"1.1 < 2.1", true},
        et {"1.1 > 2.1", false},
        et {"1.1 < 1.1", false},
        et {"1 > 1.1", false},
        et {"1.1 > 2", false},
        et {"2 < 1.1", false},
        et {"1.1 < 2", true},
        et {"1 == 1", true},
        et {"1 != 1", false},
        et {"1 == 2", false},
        et {"1 != 2", true},
        et {"1 == 1.0", true},
        et {"1 != 1.0", false},
        et {"1 == 2.0", false},
        et {"1 != 2.0", true},
        et {"1.0 == 1", true},
        et {"1.0 != 1", false},
        et {"1.0 == 2", false},
        et {"1.0 != 2", true},
        et {R"(false > true)", false},
        et {R"(false < true)", true},
        et {R"("a" < "b")", true},
        et {R"("a" > "b")", false},
        et {R"("a" < "a")", false},
        et {R"("a" > "a")", false},
        et {R"("a" == "a")", true},
        et {R"("a" != "a")", false},
        et {R"("a" == "b")", false},
        et {R"("a" != "b")", true},
        et {R"([1] == [1, 2])", false},
        et {R"([1] == [1])", true},
        et {R"([1] != [2])", true},
        et {R"([1] != [1, 2])", true},
        et {R"({1: 2} == {1: 2})", true},
        et {R"({2: 1} != {1: 2, 2: 3})", true},
        et {R"({2: 1} != {1: 2})", true},
        et {R"({2: 1} == [1, 2])", false},
        et {R"(1 == [1])", false},
        et {R"("1" == ["1"])", false},
        et {"false & false", false},
        et {"true & false", false},
        et {"true & true", true},
        et {"false | false", false},
        et {"true | false", true},
        et {"true | true", true},
        et {"false ^ false", false},
        et {"true ^ false", true},
        et {"true ^ true", false},
        et {"false && false", false},
        et {"true && false", false},
        et {"true && true", true},
        et {"false && 0", false},
        et {"true && 1", true},
        et {R"("a" && true)", true},
        et {"[1] && true", true},
        et {"[] && true", false},
        et {"true && {}", false},
    };
    for (const auto& [input, expected] : tests) {
        const auto evaluated = run(input);
        require_eq(evaluated, expected, input);
    }
}

TEST_CASE("stringExpression")
{
    const auto* input = R"("Hello World!")";
    auto evaluated = run(input);
    require_eq(evaluated, std::string("Hello World!"), input);
}

TEST_CASE("stringConcatenation")
{
    auto input = R"("Hello" + " " + "World!")";
    auto evaluated = run(input);
    require_eq(evaluated, std::string("Hello World!"), input);
}

TEST_CASE("stringIntegerMultiplication")
{
    auto input = R"("Hello" * 2 + " " + 3 * "World!")";
    auto evaluated = run(input);
    require_eq(evaluated, std::string("HelloHello World!World!World!"), input);
}

TEST_CASE("bangOperator")
{
    struct et
    {
        std::string_view input;
        bool expected;
    };

    std::array tests {
        et {"!true", false},
        et {"!false", true},
        et {"!false", true},
        et {"!0", true},
        et {"!5", false},
        et {R"(!"a")", false},
        et {R"(!"")", true},
        et {"!!true", true},
        et {"!!false", false},
        et {"!!5", true},
        et {R"(!!"a")", true},
        et {R"(![])", true},
        et {R"(!{})", true},
    };
    for (const auto& [input, expected] : tests) {
        const auto evaluated = run(input);
        require_eq(evaluated, expected, input);
    }
}

TEST_CASE("ifElseExpressions")
{
    struct et
    {
        std::string_view input;
        std::variant<null_type, int64_t> expected;
    };

    std::array tests {
        et {"if (true) { 10 }", 10},
        et {"if (false) { 10 }", null_value},
        et {"if (1) { 10 }", 10},
        et {"if (1 < 2) { 10 }", 10},
        et {"if (1 > 2) { 10 }", null_value},
        et {"if (1 > 2) { 10 } else { 20 }", 20},
        et {"if (1 < 2) { 10 } else { 20 }", 10},
    };
    for (const auto& test : tests) {
        const auto evaluated = run(test.input);
        std::visit(
            overloaded {
                [&](const null_type& /*null*/) { REQUIRE(evaluated->is(object::object_type::null)); },
                [&](const int64_t value) { require_eq(evaluated, value, test.input); },
            },
            test.expected);
    }
}

TEST_CASE("returnStatements")
{
    struct rt
    {
        std::string_view input;
        int64_t expected;
    };

    std::array tests {
        rt {"return 10;", 10},
        rt {"return 10; 9;", 10},
        rt {"return 2 * 5; 9;", 10},
        rt {"9; return 2 * 5; 9;", 10},
        rt {R"r(
if (10 > 1) {
    if (10 > 1) {
        return 10;
    }
    return 1;
})r",
            10},
    };
    for (const auto& [input, expected] : tests) {
        const auto evaluated = run(input);
        require_eq(evaluated, expected, input);
    }
}

TEST_CASE("errorHandling")
{
    struct et
    {
        std::string_view input;
        std::string expected_message;
    };

    std::array tests {
        et {
            "5 / 0;",
            "division by zero",
        },
        et {
            "5 + true;",
            "type mismatch: integer + boolean",
        },
        et {
            "5 + true; 5;",
            "type mismatch: integer + boolean",
        },
        et {
            "-true",
            "unknown operator: -boolean",
        },
        et {
            "true + false;",
            "unknown operator: boolean + boolean",
        },
        et {
            "5; true + false; 5",
            "unknown operator: boolean + boolean",
        },
        et {
            "if (10 > 1) { true + false; }",
            "unknown operator: boolean + boolean",
        },
        et {
            R"r(
if (10 > 1) {
if (10 > 1) {
return true + false;
}
return 1;
}
   )r",
            "unknown operator: boolean + boolean",
        },
        et {
            "foobar",
            "identifier not found: foobar",
        },
        et {
            R"("Hello" - "World")",
            "unknown operator: string - string",
        },
        et {
            R"({"name": "Monkey"}[fn(x) { x }];)",
            "unusable as hash key: function",
        },
    };

    for (const auto& [input, expected] : tests) {
        const auto evaluated = run(input);
        require_error_eq(evaluated, expected, input);
    }
}

TEST_CASE("integerLetStatements")
{
    struct lt
    {
        std::string_view input;
        int64_t expected;
    };

    std::array tests {
        lt {"let a = 5; a;", 5},
        lt {"let a = 5 * 5; a;", 25},
        lt {"let a = 5; let b = a; b;", 5},
        lt {"let a = 5; let b = a; let c = a + b + 5; c;", 15},
    };

    for (const auto& [input, expected] : tests) {
        require_eq(run(input), expected, input);
    }
}

TEST_CASE("boundFunction")
{
    const auto* input = "fn(x) {x + 2; };";
    auto evaluated = run(input);
    INFO("expected a function object, got ", evaluated->inspect());
    REQUIRE(evaluated->is(object::object_type::function));
}

TEST_CASE("functionApplication")
{
    struct ft
    {
        std::string_view input;
        int64_t expected;
    };

    std::array tests {
        ft {"let identity = fn(x) { x; }; identity(5);", 5},
        ft {"let identity = fn(x) { return x; }; identity(5);", 5},
        ft {"let double = fn(x) { x * 2; }; double(5);", 10},
        ft {"let add = fn(x, y) { x + y; }; add(5, 5);", 10},
        ft {"let add = fn(x, y) { x + y; }; add(5 + 5, add(5, 5));", 20},
        ft {"fn(x) { x; }(5)", 5},
        ft {"let c = fn(x) { x + 2; }; c(2 + c(4))", 10},
    };
    for (const auto& [input, expected] : tests) {
        require_eq(run(input), expected, input);
    }
}

TEST_CASE("multipleEvaluationsWithSameEnvAndDestroyedSources")
{
    const auto* input1 {R"(let makeGreeter = fn(greeting) { fn(name) { greeting + " " + name + "!" } };)"};
    const auto* input2 {R"(let hello = makeGreeter("hello");)"};
    const auto* input3 {R"(hello("banana");)"};
    std::deque<std::string> inputs {input1, input2, input3};
    require_eq(run_multi(inputs), std::string("hello banana!"), fmt::format("{}", fmt::join(inputs, "\n")));
}

TEST_CASE("builtinFunctions")
{
    struct bt
    {
        std::string_view input;
        std::variant<std::int64_t, std::string, error, null_type, array> expected;
    };

    const std::array tests {
        bt {R"(len(""))", 0},
        bt {R"(len("four"))", 4},
        bt {R"(len("hello world"))", 11},
        bt {R"(len(1))", error {"argument of type integer to len() is not supported"}},
        bt {R"(len("one", "two"))", error {"wrong number of arguments to len(): expected=1, got=2"}},
        bt {R"(len([1,2]))", 2},
        bt {R"(len({1: 2, 2: 3}))", 2},
        bt {R"(first("abc"))", "a"},
        bt {R"(first())", error {"wrong number of arguments to first(): expected=1, got=0"}},
        bt {R"(first(1))", error {"argument of type integer to first() is not supported"}},
        bt {R"(first([1,2]))", 1},
        bt {R"(first([]))", null_value},
        bt {R"(last("abc"))", "c"},
        bt {R"(last())", error {"wrong number of arguments to last(): expected=1, got=0"}},
        bt {R"(last(1))", error {"argument of type integer to last() is not supported"}},
        bt {R"(last([1,2]))", 2},
        bt {R"(last([]))", null_value},
        bt {R"(rest("abc"))", "bc"},
        bt {R"(rest("bc"))", "c"},
        bt {R"(rest("c"))", null_value},
        bt {R"(rest())", error {"wrong number of arguments to rest(): expected=1, got=0"}},
        bt {R"(rest(1))", error {"argument of type integer to rest() is not supported"}},
        bt {R"(rest([1,2]))", array {{2}}},
        bt {R"(rest([1]))", null_value},
        bt {R"(rest([]))", null_value},
        bt {R"(push())", error {"wrong number of arguments to push(): expected=2, got=0"}},
        bt {R"(push(1))", error {"wrong number of arguments to push(): expected=2, got=1"}},
        bt {R"(push(1, 2))", error {"argument of type integer and integer to push() are not supported"}},
        bt {R"(push([1,2], 3))", array {{1}, {2}, {3}}},
        bt {R"(push([], "abc"))", array {{"abc"}}},
        bt {R"(push("", "a"))", "a"},
        bt {R"(push("c", "abc"))", "cabc"},
        bt {R"(type("c"))", "string"},
        bt {R"(type())", error {"wrong number of arguments to type(): expected=1, got=0"}},
        bt {R"(type(type))", {"builtin"}},
    };

    for (const auto& test : tests) {
        auto evaluated = run(test.input);
        std::visit(
            overloaded {
                [&](const int64_t val) { require_eq(evaluated, val, test.input); },
                [&](const error& val) { require_error_eq(evaluated, val.message, test.input); },
                [&](const std::string& val) { require_eq(evaluated, val, test.input); },
                [&](const array& val) { require_array_eq(evaluated, val, test.input); },
                [&](const null_type& /*val*/) { REQUIRE(evaluated->is(object::object_type::null)); },
            },
            test.expected);
    }
}

TEST_CASE("arrayExpression")
{
    struct at
    {
        std::string_view input;
        array expected;
    };

    std::array tests {
        at {
            "[1, 2 * 2, 3 + 3]",
            {{1}, 4, 6},
        },
        at {
            "2 * [1, 3, 4]",
            {{1}, {3}, {4}, {1}, {3}, {4}},
        },
        at {
            "[1, 3] * 3",
            {{1}, {3}, {1}, {3}, {1}, {3}},
        },
        at {
            "[1, 3] * 0",
            {},
        },
        at {
            "-1 * [1, 3]",
            {},
        },
        at {
            "[2] + [1, 3]",
            {{2}, {1}, {3}},
        },
    };
    for (const auto& test : tests) {
        const auto evaluated = run(test.input);
        INFO("got: " << evaluated->type() << " for " << test.input);
        REQUIRE(evaluated->is(object::object_type::array));
        require_array_eq(evaluated->as<array_object>(), test.expected, test.input);
    }
}

TEST_CASE("indexOperatorExpressions")
{
    struct it
    {
        std::string_view input;
        std::variant<int64_t, std::string, null_type> expected;
    };

    std::array tests {
        it {
            "[1, 2, 3][0]",
            1,
        },
        it {
            "[1, 2, 3][1]",
            2,
        },
        it {
            "[1, 2, 3][2]",
            3,
        },
        it {
            "let i = 0; [1][i];",
            1,
        },
        it {
            "[1, 2, 3][1 + 1];",
            3,
        },
        it {
            "let myArray = [1, 2, 3]; myArray[2];",
            3,
        },
        it {
            "let myArray = [1, 2, 3]; myArray[0] + myArray[1] + myArray[2];",
            6,
        },
        it {
            "let myArray = [1, 2, 3]; let i = myArray[0]; myArray[i]",
            2,
        },
        it {
            "[1, 2, 3][3]",
            null_value,
        },
        it {
            "[1, 2, 3][-1]",
            null_value,
        },
        it {
            R"("2"[0])",
            "2",
        },
        it {
            R"(""[0])",
            null_value,
        },
    };

    for (const auto& test : tests) {
        auto evaluated = run(test.input);
        INFO("got: ", evaluated->type(), " with: ", evaluated->inspect());
        std::visit(
            overloaded {
                [&](const int64_t val) { require_eq(evaluated, val, test.input); },
                [&](const error& val) { require_error_eq(evaluated, val.message, test.input); },
                [&](const std::string& val) { require_eq(evaluated, val, test.input); },
                [&](const array& val) { require_array_eq(evaluated, val, test.input); },
                [&](const null_type& /*val*/) { REQUIRE(evaluated->is(object::object_type::null)); },
            },
            test.expected);
    }
}

TEST_CASE("hashLiterals")
{
    auto evaluated = run(R"(

    let two = "two";
    {
        "one": 10 - 9,
        two: 1 + 1,
        "thr" + "ee": 6 / 2,
        4: 3
    } + {
        4: 4,
        true: 5,
        false: 6
    }
        )");

    INFO("expected hash, got: " << evaluated->type());
    REQUIRE(evaluated->is(object::object_type::hash));

    struct expect
    {
        hashable_object::hash_key_type key;
        int64_t value;
    };

    std::array expected {
        expect {"one", 1},
        expect {"two", 2},
        expect {"three", 3},
        expect {4, 4},
        expect {true, 5},
        expect {false, 6},
    };
    const auto& as_hash = evaluated->as<hash_object>()->value;
    for (const auto& [key, value] : expected) {
        REQUIRE(as_hash.contains(key));
        REQUIRE_EQ(value, as_hash.at(key)->as<integer_object>()->value);
    }
}

TEST_CASE("hashIndexExpression")
{
    struct ht
    {
        std::string_view input;
        std::variant<null_type, int64_t> expected;
    };

    std::array tests {
        ht {
            R"({"foo": 5}["foo"])",
            5,
        },
        ht {
            R"({"foo": 5}["bar"])",
            null_value,
        },
        ht {
            R"(let key = "foo"; {"foo": 5}[key])",
            5,
        },
        ht {
            R"({}["foo"])",
            null_value,
        },
        ht {
            R"({5: 5}[5])",
            5,
        },
        ht {
            R"({true: 5}[true])",
            5,
        },
        ht {
            R"({false: 5}[false])",
            5,
        },
    };

    for (const auto& [input, expected] : tests) {
        auto evaluated = run(input);
        REQUIRE_FALSE(evaluated->is_error());
        std::visit(
            overloaded {
                [&](const null_type&) { CHECK(evaluated->is(object::object_type::null)); },
                [&](const int64_t value)
                {
                    REQUIRE(evaluated->is(object::object_type::integer));
                    CHECK_EQ(evaluated->as<integer_object>()->value, value);
                },
            },
            expected);
    }
}

TEST_SUITE_END();

// NOLINTEND(*)
}  // namespace
