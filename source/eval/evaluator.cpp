#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "evaluator.hpp"

#include <ast/array_literal.hpp>
#include <ast/assign_expression.hpp>
#include <ast/binary_expression.hpp>
#include <ast/boolean_literal.hpp>
#include <ast/call_expression.hpp>
#include <ast/decimal_literal.hpp>
#include <ast/expression.hpp>
#include <ast/function_literal.hpp>
#include <ast/hash_literal.hpp>
#include <ast/identifier.hpp>
#include <ast/if_expression.hpp>
#include <ast/index_expression.hpp>
#include <ast/integer_literal.hpp>
#include <ast/program.hpp>
#include <ast/statements.hpp>
#include <ast/string_literal.hpp>
#include <ast/unary_expression.hpp>
#include <builtin/builtin.hpp>
#include <doctest/doctest.h>
#include <fmt/base.h>
#include <fmt/ranges.h>
#include <gc.hpp>
#include <lexer/token_type.hpp>
#include <object/object.hpp>
#include <overloaded.hpp>
#include <parser/parser.hpp>

#include "environment.hpp"

evaluator::evaluator(environment* existing_env)
    : m_env {existing_env != nullptr ? existing_env : make<environment>()}
    , m_result {null()}
{
}

auto evaluator::evaluate(const program* prgrm) -> const object*
{
    prgrm->accept(*this);
    return m_result;
}

void evaluator::visit(const array_literal& expr)
{
    array_object::value_type arr;
    for (const auto& element : expr.elements) {
        element->accept(*this);
        if (m_result->is_error()) {
            return;
        }
        arr.push_back(m_result);
    }
    m_result = make<array_object>(std::move(arr));
}

void evaluator::visit(const assign_expression& expr)
{
    expr.value->accept(*this);
    if (m_result->is_error()) {
        return;
    }
    m_env->reassign(expr.name->value, m_result);
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
            return *right > *left;
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
        case logical_or:
            return *left || *right;
        case double_slash:
            return object_floor_div(left, right);
        default:
            return {};
    }
}

}  // namespace

void evaluator::visit(const binary_expression& expr)
{
    expr.left->accept(*this);
    if (m_result->is_error()) {
        return;
    }
    const object* evaluated_left = m_result;
    expr.right->accept(*this);
    if (m_result->is_error()) {
        return;
    }
    const object* evaluated_right = m_result;

    if (const auto* val = apply_binary_operator(expr.op, evaluated_left, evaluated_right); val != nullptr) {
        m_result = val;
        return;
    }
    if (evaluated_left->type() != evaluated_right->type()) {
        m_result = make_error("type mismatch: {} {} {}", evaluated_left->type(), expr.op, evaluated_right->type());
        return;
    }

    m_result = make_error("unknown operator: {} {} {}", evaluated_left->type(), expr.op, evaluated_right->type());
}

void evaluator::visit(const boolean_literal& expr)
{
    m_result = native_bool_to_object(expr.value);
}

void evaluator::visit(const hash_literal& expr)
{
    hash_object::value_type result;
    for (const auto& [key, value] : expr.pairs) {
        key->accept(*this);
        const auto* eval_key = m_result;
        if (eval_key->is_error()) {
            return;
        }
        if (!eval_key->is_hashable()) {
            m_result = make_error("unusable as hash key {}", eval_key->type());
            return;
        }
        value->accept(*this);
        const auto* eval_val = m_result;
        if (eval_val->is_error()) {
            return;
        }
        result.insert({eval_key->as<hashable_object>()->hash_key(), eval_val});
    }
    m_result = make<hash_object>(std::move(result));
}

void evaluator::visit(const identifier& expr)
{
    const auto* val = m_env->get(expr.value);
    if (val->is_null()) {
        m_result = make_error("identifier not found: {}", expr.value);
        return;
    }
    m_result = val;
}

void evaluator::visit(const if_expression& expr)
{
    expr.condition->accept(*this);
    const auto* evaluated_condition = m_result;
    if (evaluated_condition->is_error()) {
        return;
    }
    if (evaluated_condition->is_truthy()) {
        expr.consequence->accept(*this);
        return;
    }
    if (expr.alternative != nullptr) {
        expr.alternative->accept(*this);
        return;
    }
    m_result = null();
}

void evaluator::visit(const while_statement& expr)
{
    while (true) {
        expr.condition->accept(*this);
        const auto* evaluated_condition = m_result;
        if (evaluated_condition->is_error()) {
            return;
        }
        if (evaluated_condition->is_truthy()) {
            expr.body->accept(*this);
            const auto* result = m_result;
            if (result->is_error() || result->is_return_value()) {
                return;
            }
            if (result->is_break()) {
                break;
            }
            if (result->is_continue()) {
                continue;
            }
        } else {
            break;
        }
    }
    m_result = null();
}

void evaluator::visit(const index_expression& expr)
{
    expr.left->accept(*this);
    const auto* evaluated_left = m_result;
    if (evaluated_left->is_error()) {
        return;
    }
    expr.index->accept(*this);
    const auto* evaluated_index = m_result;
    if (evaluated_index->is_error()) {
        return;
    }
    using enum object::object_type;
    if (evaluated_left->is(array) && evaluated_index->is(integer)) {
        const auto& arr = evaluated_left->as<array_object>()->value;
        auto index = evaluated_index->as<integer_object>()->value;
        auto max = static_cast<int64_t>(arr.size() - 1);
        if (index < 0 || index > max) {
            m_result = null();
            return;
        }
        m_result = arr[static_cast<std::size_t>(index)];
        return;
    }

    if (evaluated_left->is(string) && evaluated_index->is(integer)) {
        const auto& str = evaluated_left->as<string_object>()->value;
        auto index = evaluated_index->as<integer_object>()->value;
        auto max = static_cast<int64_t>(str.size() - 1);
        if (index < 0 || index > max) {
            m_result = null();
            return;
        }
        m_result = make<string_object>(str.substr(static_cast<std::size_t>(index), 1));
        return;
    }

    if (evaluated_left->is(hash)) {
        const auto& hsh = evaluated_left->as<hash_object>()->value;
        if (!evaluated_index->is_hashable()) {
            m_result = make_error("unusable as hash key: {}", evaluated_index->type());
            return;
        }
        const auto hash_key = evaluated_index->as<hashable_object>()->hash_key();
        if (const auto itr = hsh.find(hash_key); itr != hsh.end()) {
            m_result = itr->second;
            return;
        }
        m_result = null();
        return;
    }
    m_result = make_error("index operator not supported: {}", evaluated_left->type());
}

void evaluator::visit(const integer_literal& expr)
{
    m_result = make<integer_object>(expr.value);
}

void evaluator::visit(const decimal_literal& expr)
{
    m_result = make<decimal_object>(expr.value);
}

void evaluator::visit(const program& expr)
{
    for (const auto* statement : expr.statements) {
        statement->accept(*this);
        if (m_result->is_error()) {
            return;
        }
        if (m_result->is_return_value()) {
            m_result = m_result->as<return_value_object>()->return_value;
            return;
        }
    }
}

void evaluator::visit(const let_statement& expr)
{
    expr.value->accept(*this);
    if (m_result->is_error()) {
        return;
    }
    m_env->set(expr.name->value, m_result);
    m_result = null();
}

void evaluator::visit(const return_statement& expr)
{
    if (expr.value != nullptr) {
        expr.value->accept(*this);
        const auto* evaluated = m_result;
        if (evaluated->is_error()) {
            return;
        }
        m_result = make<return_value_object>(evaluated);
        return;
    }
    m_result = null();
}

void evaluator::visit(const break_statement& /*expr*/)
{
    m_result = brake();
}

void evaluator::visit(const continue_statement& /*expr*/)
{
    m_result = cont();
}

void evaluator::visit(const expression_statement& expr)
{
    if (expr.expr != nullptr) {
        expr.expr->accept(*this);
        return;
    }
    m_result = null();
}

void evaluator::visit(const block_statement& expr)
{
    for (const auto* stmt : expr.statements) {
        stmt->accept(*this);
        if (m_result->is_error() || m_result->is_return_value() || m_result->is_break() || m_result->is_continue()) {
            return;
        }
    }
}

void evaluator::visit(const string_literal& expr)
{
    m_result = make<string_object>(expr.value);
}

void evaluator::visit(const unary_expression& expr)
{
    expr.right->accept(*this);
    const auto* evaluated_value = m_result;
    if (evaluated_value->is_error()) {
        return;
    }
    using enum token_type;
    switch (expr.op) {
        case minus:
            if (evaluated_value->is(object::object_type::integer)) {
                m_result = make<integer_object>(-evaluated_value->as<integer_object>()->value);
                return;
            } else if (evaluated_value->is(object::object_type::decimal)) {
                m_result = make<decimal_object>(-evaluated_value->as<decimal_object>()->value);
                return;
            }
            m_result = make_error("unknown operator: -{}", evaluated_value->type());
            return;
        case exclamation:
            m_result = native_bool_to_object(!evaluated_value->is_truthy());
            return;
        default:
            m_result = make_error("unknown operator: {}{}", expr.op, evaluated_value->type());
            return;
    }
}

void evaluator::visit(const call_expression& expr)
{
    expr.function->accept(*this);
    if (m_result->is_error()) {
        return;
    }
    const auto* func = m_result;
    auto args = evaluate_expressions(expr.arguments);
    if (m_result->is_error()) {
        return;
    }
    apply_function(func, std::move(args));
}

void evaluator::visit(const function_literal& expr)
{
    m_result = make<function_object>(expr.parameters, expr.body, m_env);
}

void evaluator::apply_function(const object* function_or_builtin, array_object::value_type&& args)
{
    if (function_or_builtin->is(object::object_type::function)) {
        const auto* func = function_or_builtin->as<function_object>();
        auto* locals = make<environment>(func->closure_env);
        for (auto arg_itr = args.begin(); const auto* parameter : func->parameters) {
            locals->set(parameter->value, *(arg_itr++));
        }
        {
            evaluator local(locals);
            func->body->accept(local);
            m_result = local.m_result;
        }
        if (m_result->is_return_value()) {
            m_result = m_result->as<return_value_object>()->return_value;
        }
        return;
    }
    if (function_or_builtin->is(object::object_type::builtin)) {
        const auto* builtin = function_or_builtin->as<builtin_object>();
        m_result = builtin->builtin->body(std::move(args));
        return;
    }
    m_result = make_error("not a function {}", m_result->type());
}

auto evaluator::evaluate_expressions(const expressions& exprs) -> array_object::value_type
{
    array_object::value_type result;
    for (const auto* expr : exprs) {
        expr->accept(*this);
        if (m_result->is_error()) {
            return {m_result};
        }
        result.push_back(m_result);
    }
    return result;
}

namespace
{
namespace dt = doctest;

struct error
{
    std::string message;
};

using array = std::vector<std::variant<std::string, int64_t>>;
using hash = std::unordered_map<std::string, int64_t>;
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
    INFO(input, " expected: decimal with: ", expected, " got: ", obj->type(), " with: ", obj->inspect());
    REQUIRE(obj->is(object::object_type::decimal));
    const auto& actual = obj->as<decimal_object>()->value;
    REQUIRE(actual == dt::Approx(expected));
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

auto require_hash_eq(const object* obj, const hash& expected, std::string_view input) -> void
{
    INFO(input, " expected: hash with: ", expected.size(), " got: ", obj->type(), " with: ", obj->inspect());
    REQUIRE(obj->is(object::object_type::hash));
    const auto& actual = obj->as<hash_object>()->value;
    REQUIRE(actual.size() == expected.size());
    for (const auto& [expected_key, expected_value] : expected) {
        REQUIRE(actual.contains(expected_key));
        auto val = actual.at(expected_key);
        require_eq(val, expected_value, input);
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
    for (const auto& builtin : builtin::builtins()) {
        env.set(builtin->name, make<builtin_object>(builtin));
    }
    evaluator ev(&env);
    auto result = ev.evaluate(prgrm);
    return result;
}

auto run_multi(std::deque<std::string>& inputs) -> const object*
{
    environment env;
    const object* result = nullptr;
    while (!inputs.empty()) {
        auto [prgrm, _] = check_program(inputs.front());
        evaluator ev {&env};
        result = ev.evaluate(prgrm);
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
        et {"2 * (5 + 10)", 30},
        et {"3 * 3 * 3 + 10", 37},
        et {"3 * (3 * 3) + 10", 37},
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
        et {"5.0 + 5 + 5 + 5 - 10", 10.0},
        et {"2.0 * 2 * 2 * 2 * 2", 32.0},
        et {"-50.0 + 100 + -50", 0.0},
        et {"5.0 * 2 + 10", 20.0},
        et {"5.0 + 2 * 10", 25.0},
        et {"20.0 + 2.0 * -10.0", 0.0},
        et {"50 / 2 * 2 + 10", 60.0},
        et {"2.0 * (5 + 10)", 30.0},
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
        et {"10 // 2", 5.0},
        et {"-5 // -2", 2.0},
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
        et {"false || false", false},
        et {"true || false", true},
        et {"true || true", true},
        et {"false || 0", false},
        et {"true || 1", true},
        et {R"("a" || true)", true},
        et {"[1] || true", true},
        et {"[] || true", true},
        et {"true || {}", true},
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
                [&](const null_type& /*null*/) { REQUIRE(evaluated->is(object::object_type::nll)); },
                [&](const int64_t value) { require_eq(evaluated, value, test.input); },
            },
            test.expected);
    }
}

TEST_CASE("assignExpressions")
{
    struct et
    {
        std::string_view input;
        std::variant<null_type, int64_t> expected;
    };

    std::array tests {
        et {R"(let x = 1; if (false) { x = x + 1; } x)", 1},
        et {R"(let x = 1; let y = 1; if (y > 0) { y = y - 1; x = x + 1; } x)", 2},
        et {R"(let a = 6; let b = a; let x = 1; x = x - 1; a = 5; b = b + a;)", 11},
    };

    for (const auto& test : tests) {
        const auto evaluated = run(test.input);
        std::visit(
            overloaded {
                [&](const null_type& /*null*/) { REQUIRE(evaluated->is(object::object_type::nll)); },
                [&](const int64_t value) { require_eq(evaluated, value, test.input); },
            },
            test.expected);
    }
}

TEST_CASE("whileStatements")
{
    struct et
    {
        std::string_view input;
        std::variant<null_type, int64_t, double> expected;
    };

    std::array tests {
        et {R"(let x = 1; while (false) { x = x + 1; } x)", 1},
        et {R"(let x = 1; let y = 1; while (y > 0) { y = y - 1; x = x + 1; } x)", 2},
        et {R"(let a = 6;
                          let b = a;
                          let x = 1;
                          while (x > 0) {
                              x = x - 1;
                              a = 5;
                              b = b + a;
                              let c = 8;
                              c = c / 2;
                              b = b + c;
                              let y = 1;
                              while (y > 0) {
                                  y = y - 1;
                                  a = a + 3;
                                  b = b + a;
                              }
                          }
                          b = b + a;)",
            31.0},
    };

    for (const auto& test : tests) {
        const auto evaluated = run(test.input);
        std::visit(
            overloaded {
                [&](const null_type& /*null*/) { REQUIRE(evaluated->is(object::object_type::nll)); },
                [&](const int64_t value) { require_eq(evaluated, value, test.input); },
                [&](const double value) { require_eq(evaluated, value, test.input); },
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
            R"(5 + "str";)",
            "type mismatch: integer + string",
        },
        et {
            R"(5 + "str"; 5;)",
            "type mismatch: integer + string",
        },
        et {
            "-true",
            "unknown operator: -boolean",
        },
        et {
            R"(5; true + "str"; 5)",
            "type mismatch: boolean + string",
        },
        et {
            R"(if (10 > 1) { true + "str"; })",
            "type mismatch: boolean + string",
        },
        et {
            R"r(
if (10 > 1) {
if (10 > 1) {
return true + "str";
}
return 1;
}
   )r",
            "type mismatch: boolean + string",
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
            R"({"name": "Cappuchin"}[fn(x) { x }];)",
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
    const auto* input = "fn(x) { x + 2; };";
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
        std::variant<std::int64_t, std::string, error, null_type, array, hash> expected;
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
        bt {R"(push())", error {"wrong number of arguments to push(): expected=2 or 3, got=0"}},
        bt {R"(push(1))", error {"wrong number of arguments to push(): expected=2 or 3, got=1"}},
        bt {R"(push(1, 2))", error {"argument of type integer and integer to push() are not supported"}},
        bt {R"(push([1,2], 3))", array {{1}, {2}, {3}}},
        bt {R"(push([], "abc"))", array {{"abc"}}},
        bt {R"(push({}, "c", 1))", hash {{"c", 1}}},
        bt {R"(push("", "a"))", "a"},
        bt {R"(push("c", "abc"))", "cabc"},
        bt {R"(type("c"))", "string"},
        bt {R"(type())", error {"wrong number of arguments to type(): expected=1, got=0"}},
        bt {R"(type(type))", {"builtin"}},
        bt {R"(chr())", error {"wrong number of arguments to chr(): expected=1, got=0"}},
        bt {R"(chr("65"))", error {"argument of type string to chr() is not supported"}},
        bt {R"(chr(128))", error {"number 128 is out of range to be an ascii character"}},
        bt {R"(chr(65))", {"A"}},
    };

    for (const auto& test : tests) {
        auto evaluated = run(test.input);
        std::visit(
            overloaded {
                [&](const int64_t val) { require_eq(evaluated, val, test.input); },
                [&](const error& val) { require_error_eq(evaluated, val.message, test.input); },
                [&](const std::string& val) { require_eq(evaluated, val, test.input); },
                [&](const array& val) { require_array_eq(evaluated, val, test.input); },
                [&](const hash& val) { require_hash_eq(evaluated, val, test.input); },
                [&](const null_type& /*val*/) { REQUIRE(evaluated->is(object::object_type::nll)); },
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
                [&](const null_type& /*val*/) { REQUIRE(evaluated->is(object::object_type::nll)); },
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
        "twe" + "lve": 6 * 2,
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
        expect {"twelve", 12},
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
                [&](const null_type&) { CHECK(evaluated->is(object::object_type::nll)); },
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
