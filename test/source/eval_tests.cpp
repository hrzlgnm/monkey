#include <cstdint>
#include <deque>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <variant>

#include <ast/builtin_function_expression.hpp>
#include <eval/environment.hpp>
#include <eval/object.hpp>
#include <gtest/gtest.h>
#include <lexer/lexer.hpp>
#include <parser/parser.hpp>

#include "testutils.hpp"

// NOLINTBEGIN(*-magic-numbers)
auto assert_integer_object(const object& obj, int64_t expected) -> void
{
    ASSERT_TRUE(obj.is<integer_value>()) << "got " << obj.type_name() << " instead";
    auto actual = obj.as<integer_value>();
    ASSERT_EQ(actual, expected);
}

auto assert_boolean_object(const object& obj, bool expected) -> void
{
    ASSERT_TRUE(obj.is<bool>()) << "got " << obj.type_name() << " instead ";
    auto actual = obj.as<bool>();
    ASSERT_EQ(actual, expected);
}

auto assert_nil_object(const object& obj) -> void
{
    ASSERT_TRUE(obj.is_nil());
}

auto assert_string_object(const object& obj, const std::string& expected) -> void
{
    ASSERT_TRUE(obj.is<string_value>()) << "got " << obj.type_name() << " instead";
    const auto& actual = obj.as<string_value>();
    ASSERT_EQ(actual, expected);
}

auto assert_error_object(const object& obj, const std::string& expected_error_message) -> void
{
    ASSERT_TRUE(obj.is<error>()) << "got " << obj.type_name() << " instead";
    auto actual = obj.as<error>();
    EXPECT_EQ(actual.message, expected_error_message);
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

TEST(eval, testEvalIntegerExpresssion)
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

TEST(eval, testEvalBooleanExpresssion)
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

TEST(eval, testEvalStringExpression)
{
    auto evaluated = test_eval(R"("Hello World!")");
    ASSERT_TRUE(evaluated.is<string_value>());
    ASSERT_EQ(evaluated.as<string_value>(), "Hello World!");
}

TEST(eval, testEvalStringConcatenation)
{
    auto evaluated = test_eval(R"("Hello" + " " + "World!")");
    ASSERT_TRUE(evaluated.is<string_value>()) << "expected a string, got " << evaluated.type_name() << " instead";

    ASSERT_EQ(evaluated.as<string_value>(), "Hello World!");
}

TEST(eval, testBangOperator)
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

TEST(eval, testIfElseExpressions)
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
        if (test.expected.is<integer_value>()) {
            assert_integer_object(evaluated, test.expected.as<integer_value>());
        } else {
            assert_nil_object(evaluated);
        }
    }
}

TEST(eval, testReturnStatemets)
{
    struct return_test
    {
        std::string_view input;
        integer_value expected;
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

TEST(eval, testErrorHandling)
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
        EXPECT_TRUE(evaluated.is<error>())
            << test.input << ": expected an error, got " << evaluated.type_name() << " instead";
        EXPECT_EQ(evaluated.as<error>().message, test.expected_message);
    }
}

TEST(eval, testIntegerLetStatements)
{
    struct let_test
    {
        std::string_view input;
        integer_value expected;
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

TEST(eval, testFunctionObject)
{
    const auto* input = "fn(x) {x + 2; };";
    auto evaluated = test_eval(input);
    ASSERT_TRUE(evaluated.is<bound_function>())
        << "expected a function object, got " << std::to_string(evaluated.value) << " instead ";
}

TEST(eval, testFunctionApplication)
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

TEST(eval, testMultipleEvaluationsWithSameEnvAndDestroyedSources)
{
    const auto* input1 {R"(let makeGreeter = fn(greeting) { fn(name) { greeting + " " + name + "!" } };)"};
    const auto* input2 {"let hello = makeGreeter(\"hello\");"};
    const auto* input3 {"hello(\"banana\");"};
    std::deque<std::string> inputs {input1, input2, input3};
    assert_string_object(test_multi_eval(inputs), "hello banana!");
}

TEST(eval, testBuiltinFunctions)
{
    struct builtin_test
    {
        std::string_view input;
        std::variant<std::int64_t, std::string, error, nil_value, array> expected;
    };
    std::array tests {
        builtin_test {R"(len(""))", 0},
        builtin_test {R"(len("four"))", 4},
        builtin_test {R"(len("hello world"))", 11},
        builtin_test {R"(len(1))", error {"argument of type integer to len() is not supported"}},
        builtin_test {R"(len("one", "two"))", error {"wrong number of arguments to len(): expected=1, got=2"}},
        builtin_test {R"(len([1,2]))", 2},
        builtin_test {R"(first("abc"))", "a"},
        builtin_test {R"(first())", error {"wrong number of arguments to first(): expected=1, got=0"}},
        builtin_test {R"(first(1))", error {"argument of type integer to first() is not supported"}},
        builtin_test {R"(first([1,2]))", 1},
        builtin_test {R"(first([]))", nil_value {}},
        builtin_test {R"(last("abc"))", "c"},
        builtin_test {R"(last())", error {"wrong number of arguments to last(): expected=1, got=0"}},
        builtin_test {R"(last(1))", error {"argument of type integer to last() is not supported"}},
        builtin_test {R"(last([1,2]))", 2},
        builtin_test {R"(last([]))", nil_value {}},
        builtin_test {R"(rest("abc"))", "bc"},
        builtin_test {R"(rest())", error {"wrong number of arguments to rest(): expected=1, got=0"}},
        builtin_test {R"(rest(1))", error {"argument of type integer to rest() is not supported"}},
        builtin_test {R"(rest([1,2]))", array {{2}}},
        builtin_test {R"(rest([1]))", nil_value {}},
        builtin_test {R"(rest([]))", nil_value {}},
        builtin_test {R"(push())", error {"wrong number of arguments to push(): expected=2, got=0"}},
        builtin_test {R"(push(1))", error {"wrong number of arguments to push(): expected=2, got=1"}},
        builtin_test {R"(push(1, 2))", error {"argument of type integer and integer to push() are not supported"}},
        builtin_test {R"(push([1,2], 3))", array {{1}, {2}, {3}}},
        builtin_test {R"(push([], "abc"))", array {{"abc"}}},
    };

    for (auto test : tests) {
        auto evaluated = test_eval(test.input);
        std::visit(overloaded {[&evaluated](const integer_value val) { assert_integer_object(evaluated, val); },
                               [&evaluated](const error& val) { assert_error_object(evaluated, val.message); },
                               [&evaluated](const std::string& val) { assert_string_object(evaluated, val); },
                               [&evaluated](const array& val) { ASSERT_EQ(object {val}, evaluated); },
                               [&evaluated](const nil_value& /*val*/) { assert_nil_object(evaluated); },
                               [](const auto& /*val*/) { FAIL(); }},
                   test.expected);
    }
}
TEST(eval, testArrayExpression)
{
    auto evaluated = test_eval("[1, 2 * 2, 3 + 3]");
    ASSERT_TRUE(evaluated.is<array>()) << "got: " << evaluated.type_name() << " instead";
    auto as_arr = evaluated.as<array>();
    assert_integer_object(as_arr[0], 1);
    assert_integer_object(as_arr[1], 4);
    assert_integer_object(as_arr[2], 6);
}

TEST(eval, testIndexOperatorExpressions)
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
            nil_value {},
        },
        test {
            "[1, 2, 3][-1]",
            nil_value {},
        },
    };

    for (const auto& [input, expected] : tests) {
        auto evaluated = test_eval(input);
        EXPECT_EQ(object {evaluated.value}, object {expected})
            << "expected " << std::to_string(expected) << ", got " << evaluated.type_name();
    }
}
TEST(eval, hashLiterals)
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

    ASSERT_TRUE(evaluated.is<hash>()) << "expected hash, got " << evaluated.type_name() << " instead";
    struct expect
    {
        hash_key_type key;
        int64_t value;
    };
    std::array expected {
        expect {"one", 1}, expect {"two", 2}, expect {"three", 3}, expect {4, 4}, expect {true, 5}, expect {false, 6}};
    const auto& as_hash = evaluated.as<hash>();
    for (const auto& [key, value] : expected) {
        ASSERT_TRUE(as_hash.contains(key));
        ASSERT_EQ(value, std::any_cast<object>(as_hash.at(key)).as<integer_value>());
    };
}

TEST(eval, hashIndexExpression)
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
            nil,
        },
        hash_test {
            R"(let key = "foo"; {"foo": 5}[key])",
            5,
        },
        hash_test {
            R"({}["foo"])",
            nil,
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
        EXPECT_FALSE(evaluated.is<error>());
        EXPECT_EQ(evaluated, object {expected});
    }
}
// NOLINTEND(*-magic-numbers)
