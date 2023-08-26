#include <deque>

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
#include <doctest/doctest.h>
#include <parser/parser.hpp>

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

namespace
{
// NOLINTBEGIN(*)
TEST_SUITE_BEGIN("eval");

auto assert_no_parse_errors(const parser& prsr) -> bool
{
    INFO("expected no errors, got:", fmt::format("{}", fmt::join(prsr.errors(), ", ")));
    CHECK(prsr.errors().empty());
    return !prsr.errors().empty();
}

auto assert_integer_object(const object& obj, int64_t expected) -> void
{
    INFO("got", obj.type_name());
    REQUIRE(obj.is<integer_type>());
    auto actual = obj.as<integer_type>();
    REQUIRE_EQ(actual, expected);
}

auto assert_boolean_object(const object& obj, bool expected) -> void
{
    INFO("got", obj.type_name());
    REQUIRE(obj.is<bool>());
    auto actual = obj.as<bool>();
    REQUIRE_EQ(actual, expected);
}

auto assert_nil_object(const object& obj) -> void
{
    INFO("got", obj.type_name());
    REQUIRE(obj.is_nil());
}

auto assert_string_object(const object& obj, const std::string& expected) -> void
{
    INFO("got", obj.type_name());
    REQUIRE(obj.is<string_type>());
    const auto& actual = obj.as<string_type>();
    REQUIRE_EQ(actual, expected);
}

auto assert_error_object(const object& obj, const std::string& expected_error_message) -> void
{
    INFO("got", obj.type_name());
    REQUIRE(obj.is<error>());
    auto actual = obj.as<error>();
    CHECK_EQ(actual.message, expected_error_message);
}

auto test_eval(std::string_view input) -> object
{
    auto prsr = parser {lexer {input}};
    auto prgrm = prsr.parse_program();
    auto env = std::make_shared<environment>();
    for (const auto& builtin : builtin_function_expression::builtins) {
        env->set(builtin.name, object {bound_function(&builtin, environment_ptr {})});
    }

    assert_no_parse_errors(prsr);
    auto result = prgrm->eval(env);
    env->break_cycle();
    return result;
}

auto test_multi_eval(std::deque<std::string>& inputs) -> object
{
    auto locals = std::make_shared<environment>();
    auto statements = std::vector<statement_ptr>();
    object result;
    while (!inputs.empty()) {
        auto prsr = parser {lexer {inputs.front()}};
        auto prgrm = prsr.parse_program();
        assert_no_parse_errors(prsr);
        result = prgrm->eval(locals);
        for (auto& stmt : prgrm->statements) {
            statements.push_back(std::move(stmt));
        }
        inputs.pop_front();
    }
    locals->break_cycle();
    return result;
}

TEST_CASE("integerExpresssion")
{
    struct expression_test
    {
        std::string_view input;
        int64_t expected;
    };

    std::array expression_tests {
        expression_test {"5", 5},
        expression_test {"10", 10},
        expression_test {"-5", -5},
        expression_test {"-10", -10},
        expression_test {"5 + 5 + 5 + 5 - 10", 10},
        expression_test {"2 * 2 * 2 * 2 * 2", 32},
        expression_test {"-50 + 100 + -50", 0},
        expression_test {"5 * 2 + 10", 20},
        expression_test {"5 + 2 * 10", 25},
        expression_test {"20 + 2 * -10", 0},
        expression_test {"50 / 2 * 2 + 10", 60},
        expression_test {"2 * (5 + 10)", 30},
        expression_test {"3 * 3 * 3 + 10", 37},
        expression_test {"3 * (3 * 3) + 10", 37},
        expression_test {"(5 + 10 * 2 + 15 / 3) * 2 + -10", 50},
    };
    for (const auto& test : expression_tests) {
        const auto evaluated = test_eval(test.input);
        assert_integer_object(evaluated, test.expected);
    }
}

TEST_CASE("booleanExpresssion")
{
    struct expression_test
    {
        std::string_view input;
        bool expected;
    };

    std::array expression_tests {
        expression_test {"true", true},
        expression_test {"false", false},
        expression_test {"1 < 2", true},
        expression_test {"1 > 2", false},
        expression_test {"1 < 1", false},
        expression_test {"1 > 1", false},
        expression_test {"1 == 1", true},
        expression_test {"1 != 1", false},
        expression_test {"1 == 2", false},
        expression_test {"1 != 2", true},

    };
    for (const auto& test : expression_tests) {
        const auto evaluated = test_eval(test.input);
        assert_boolean_object(evaluated, test.expected);
    }
}

TEST_CASE("stringExpression")
{
    auto evaluated = test_eval(R"("Hello World!")");
    REQUIRE(evaluated.is<string_type>());
    REQUIRE_EQ(evaluated.as<string_type>(), "Hello World!");
}

TEST_CASE("stringConcatenation")
{
    auto evaluated = test_eval(R"("Hello" + " " + "World!")");
    INFO("expected a string, got: ", evaluated.type_name());
    REQUIRE(evaluated.is<string_type>());

    REQUIRE_EQ(evaluated.as<string_type>(), "Hello World!");
}

TEST_CASE("bangOperator")
{
    struct expression_test
    {
        std::string_view input;
        bool expected;
    };

    std::array expression_tests {
        expression_test {"!true", false},
        expression_test {"!false", true},
        expression_test {"!false", true},
        expression_test {"!5", false},
        expression_test {"!!true", true},
        expression_test {"!!false", false},
        expression_test {"!!5", true},
    };
    for (const auto& test : expression_tests) {
        const auto evaluated = test_eval(test.input);
        assert_boolean_object(evaluated, test.expected);
    }
}

TEST_CASE("ifElseExpressions")
{
    struct expression_test
    {
        std::string_view input;
        object expected;
    };

    std::array expression_tests {
        expression_test {"if (true) { 10 }", {10}},
        expression_test {"if (false) { 10 }", {}},
        expression_test {"if (1) { 10 }", {10}},
        expression_test {"if (1 < 2) { 10 }", {10}},
        expression_test {"if (1 > 2) { 10 }", {}},
        expression_test {"if (1 > 2) { 10 } else { 20 }", {20}},
        expression_test {"if (1 < 2) { 10 } else { 20 }", {10}},
    };
    for (const auto& test : expression_tests) {
        const auto evaluated = test_eval(test.input);
        if (test.expected.is<integer_type>()) {
            assert_integer_object(evaluated, test.expected.as<integer_type>());
        } else {
            assert_nil_object(evaluated);
        }
    }
}

TEST_CASE("returnStatemets")
{
    struct return_test
    {
        std::string_view input;
        integer_type expected;
    };

    std::array return_tests {return_test {"return 10;", 10},
                             return_test {"return 10; 9;", 10},
                             return_test {"return 2 * 5; 9;", 10},
                             return_test {"9; return 2 * 5; 9;", 10},
                             return_test {R"r(
if (10 > 1) {
    if (10 > 1) {
        return 10;
    }
    return 1;
})r",
                                          10}};
    for (const auto& test : return_tests) {
        const auto evaluated = test_eval(test.input);
        assert_integer_object(evaluated, test.expected);
    }
}

TEST_CASE("errorHandling")
{
    struct error_test
    {
        std::string_view input;
        std::string expected_message;
    };

    std::array error_tests {

        error_test {
            "5 + true;",
            "type mismatch: integer + bool",
        },
        error_test {
            "5 + true; 5;",
            "type mismatch: integer + bool",
        },
        error_test {
            "-true",
            "unknown operator: -bool",
        },
        error_test {
            "true + false;",
            "unknown operator: bool + bool",
        },
        error_test {
            "5; true + false; 5",
            "unknown operator: bool + bool",
        },
        error_test {
            "if (10 > 1) { true + false; }",
            "unknown operator: bool + bool",
        },
        error_test {
            R"r(
if (10 > 1) {
if (10 > 1) {
return true + false;
}
return 1;
}
   )r",
            "unknown operator: bool + bool",
        },
        error_test {
            "foobar",
            "identifier not found: foobar",
        },
        error_test {
            R"("Hello" - "World")",
            "unknown operator: string - string",
        },
        error_test {
            R"({"name": "Monkey"}[fn(x) { x }];)",
            "unusable as hash key: function",
        }};

    for (const auto& test : error_tests) {
        const auto evaluated = test_eval(test.input);
        INFO("expected an error, got ", evaluated.type_name());
        CHECK(evaluated.is<error>());
        CHECK_EQ(evaluated.as<error>().message, test.expected_message);
    }
}

TEST_CASE("integerLetStatements")
{
    struct let_test
    {
        std::string_view input;
        integer_type expected;
    };

    std::array let_tests {
        let_test {"let a = 5; a;", 5},
        let_test {"let a = 5 * 5; a;", 25},
        let_test {"let a = 5; let b = a; b;", 5},
        let_test {"let a = 5; let b = a; let c = a + b + 5; c;", 15},
    };

    for (const auto& test : let_tests) {
        assert_integer_object(test_eval(test.input), test.expected);
    }
}

TEST_CASE("functionObject")
{
    const auto* input = "fn(x) {x + 2; };";
    auto evaluated = test_eval(input);
    INFO("expected a function object, got ", std::to_string(evaluated.value));
    REQUIRE(evaluated.is<bound_function>());
}

TEST_CASE("functionApplication")
{
    struct func_test
    {
        std::string_view input;
        int64_t expected;
    };

    std::array func_tests {
        func_test {"let identity = fn(x) { x; }; identity(5);", 5},
        func_test {"let identity = fn(x) { return x; }; identity(5);", 5},
        func_test {"let double = fn(x) { x * 2; }; double(5);", 10},
        func_test {"let add = fn(x, y) { x + y; }; add(5, 5);", 10},
        func_test {"let add = fn(x, y) { x + y; }; add(5 + 5, add(5, 5));", 20},
        func_test {"fn(x) { x; }(5)", 5},
        func_test {"let c = fn(x) { x + 2; }; c(2 + c(4))", 10},
    };
    for (const auto test : func_tests) {
        assert_integer_object(test_eval(test.input), test.expected);
    }
}

TEST_CASE("multipleEvaluationsWithSameEnvAndDestroyedSources")
{
    const auto* input1 {R"(let makeGreeter = fn(greeting) { fn(name) { greeting + " " + name + "!" } };)"};
    const auto* input2 {R"(let hello = makeGreeter("hello");)"};
    const auto* input3 {R"(hello("banana");)"};
    std::deque<std::string> inputs {input1, input2, input3};
    assert_string_object(test_multi_eval(inputs), "hello banana!");
}

TEST_CASE("builtinFunctions")
{
    struct bt
    {
        std::string_view input;
        std::variant<std::int64_t, std::string, error, nil_type, array> expected;
    };

    std::array tests {
        bt {R"(len(""))", 0},
        bt {R"(len("four"))", 4},
        bt {R"(len("hello world"))", 11},
        bt {R"(len(1))", error {"argument of type integer to len() is not supported"}},
        bt {R"(len("one", "two"))", error {"wrong number of arguments to len(): expected=1, got=2"}},
        bt {R"(len([1,2]))", 2},
        bt {R"(first("abc"))", "a"},
        bt {R"(first())", error {"wrong number of arguments to first(): expected=1, got=0"}},
        bt {R"(first(1))", error {"argument of type integer to first() is not supported"}},
        bt {R"(first([1,2]))", 1},
        bt {R"(first([]))", nil_type {}},
        bt {R"(last("abc"))", "c"},
        bt {R"(last())", error {"wrong number of arguments to last(): expected=1, got=0"}},
        bt {R"(last(1))", error {"argument of type integer to last() is not supported"}},
        bt {R"(last([1,2]))", 2},
        bt {R"(last([]))", nil_type {}},
        bt {R"(rest("abc"))", "bc"},
        bt {R"(rest())", error {"wrong number of arguments to rest(): expected=1, got=0"}},
        bt {R"(rest(1))", error {"argument of type integer to rest() is not supported"}},
        bt {R"(rest([1,2]))", array {{2}}},
        bt {R"(rest([1]))", nil_type {}},
        bt {R"(rest([]))", nil_type {}},
        bt {R"(push())", error {"wrong number of arguments to push(): expected=2, got=0"}},
        bt {R"(push(1))", error {"wrong number of arguments to push(): expected=2, got=1"}},
        bt {R"(push(1, 2))", error {"argument of type integer and integer to push() are not supported"}},
        bt {R"(push([1,2], 3))", array {{1}, {2}, {3}}},
        bt {R"(push([], "abc"))", array {{"abc"}}},
    };

    for (auto test : tests) {
        auto evaluated = test_eval(test.input);
        std::visit(
            overloaded {
                [&evaluated](const integer_type val) { assert_integer_object(evaluated, val); },
                [&evaluated](const error& val) { assert_error_object(evaluated, val.message); },
                [&evaluated](const std::string& val) { assert_string_object(evaluated, val); },
                [&evaluated](const array& val) { REQUIRE_EQ(object {val}, evaluated); },
                [&evaluated](const nil_type& /*val*/) { assert_nil_object(evaluated); },
            },
            test.expected);
    }
}

TEST_CASE("arrayExpression")
{
    auto evaluated = test_eval("[1, 2 * 2, 3 + 3]");
    INFO("got: " << evaluated.type_name());
    REQUIRE(evaluated.is<array>());
    auto as_arr = evaluated.as<array>();
    assert_integer_object(as_arr[0], 1);
    assert_integer_object(as_arr[1], 4);
    assert_integer_object(as_arr[2], 6);
}

TEST_CASE("indexOperatorExpressions")
{
    struct test
    {
        std::string_view input;
        value_type expected;
    };

    std::array tests {
        test {
            "[1, 2, 3][0]",
            1,
        },
        test {
            "[1, 2, 3][1]",
            2,
        },
        test {
            "[1, 2, 3][2]",
            3,
        },
        test {
            "let i = 0; [1][i];",
            1,
        },
        test {
            "[1, 2, 3][1 + 1];",
            3,
        },
        test {
            "let myArray = [1, 2, 3]; myArray[2];",
            3,
        },
        test {
            "let myArray = [1, 2, 3]; myArray[0] + myArray[1] + myArray[2];",
            6,
        },
        test {
            "let myArray = [1, 2, 3]; let i = myArray[0]; myArray[i]",
            2,
        },
        test {
            "[1, 2, 3][3]",
            nil_type {},
        },
        test {
            "[1, 2, 3][-1]",
            nil_type {},
        },
    };

    for (const auto& [input, expected] : tests) {
        auto evaluated = test_eval(input);
        INFO("expected: ", std::to_string(expected));
        INFO("got: ", evaluated.type_name());
        CHECK_EQ(object {evaluated.value}, object {expected});
    }
}

TEST_CASE("hashLiterals")
{
    auto evaluated = test_eval(R"(let two = "two";
    {
        "one": 10 - 9,
        two: 1 + 1,
        "thr" + "ee": 6 / 2,
        4: 4,
        true: 5,
        false: 6
    })");

    INFO("expected hash, got: " << evaluated.type_name());
    REQUIRE(evaluated.is<hash>());

    struct expect
    {
        hash_key_type key;
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
    const auto& as_hash = evaluated.as<hash>();
    for (const auto& [key, value] : expected) {
        REQUIRE(as_hash.contains(key));
        REQUIRE_EQ(value, std::any_cast<object>(as_hash.at(key)).as<integer_type>());
    };
}

TEST_CASE("hashIndexExpression")
{
    struct hash_test
    {
        std::string_view input;
        value_type expected;
    };

    std::array inputs {
        hash_test {
            R"({"foo": 5}["foo"])",
            5,
        },
        hash_test {
            R"({"foo": 5}["bar"])",
            nilv,
        },
        hash_test {
            R"(let key = "foo"; {"foo": 5}[key])",
            5,
        },
        hash_test {
            R"({}["foo"])",
            nilv,
        },
        hash_test {
            R"({5: 5}[5])",
            5,
        },
        hash_test {
            R"({true: 5}[true])",
            5,
        },
        hash_test {
            R"({false: 5}[false])",
            5,
        },
    };
    for (const auto& [input, expected] : inputs) {
        auto evaluated = test_eval(input);
        CHECK_FALSE(evaluated.is<error>());
        CHECK_EQ(evaluated, object {expected});
    }
}

TEST_SUITE_END();

// NOLINTEND(*)
}  // namespace
