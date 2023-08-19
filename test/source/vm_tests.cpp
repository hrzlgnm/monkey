#include <cstdint>
#include <exception>
#include <stdexcept>
#include <string>
#include <string_view>

#include <compiler/compiler.hpp>
#include <compiler/vm.hpp>
#include <eval/object.hpp>
#include <gtest/gtest.h>

#include "testutils.hpp"

auto assert_bool_object(bool expected, const object& actual, std::string_view input) -> void
{
    ASSERT_TRUE(actual.is<bool>()) << input << " got " << actual.type_name() << " with " << std::to_string(actual.value)
                                   << " instead";
    auto actual_bool = actual.as<bool>();
    ASSERT_EQ(actual_bool, expected);
}

auto assert_integer_object(int64_t expected, const object& actual, std::string_view input) -> void
{
    ASSERT_TRUE(actual.is<integer_type>())
        << input << " got " << actual.type_name() << " with " << std::to_string(actual.value) << " instead ";
    auto actual_int = actual.as<integer_type>();
    ASSERT_EQ(actual_int, expected);
}

auto assert_string_object(const std::string& expected, const object& actual, std::string_view input) -> void
{
    ASSERT_TRUE(actual.is<string_type>())
        << input << " got " << actual.type_name() << " with " << std::to_string(actual.value) << " instead ";
    auto actual_str = actual.as<string_type>();
    ASSERT_EQ(actual_str, expected);
}

auto assert_array_object(const std::vector<int>& expected, const object& actual, std::string_view input) -> void
{
    ASSERT_TRUE(actual.is<array>()) << input << " got " << actual.type_name() << " with "
                                    << std::to_string(actual.value) << " instead ";
    auto actual_arr = actual.as<array>();
    ASSERT_EQ(actual_arr.size(), expected.size());
    for (auto idx = 0UL; const auto& elem : expected) {
        ASSERT_EQ(elem, actual_arr.at(idx).as<integer_type>());
        ++idx;
    }
}

auto assert_hash_object(const hash& expected, const object& actual, std::string_view input) -> void
{
    ASSERT_TRUE(actual.is<hash>()) << input << " got " << actual.type_name() << " with " << std::to_string(actual.value)
                                   << " instead ";
    auto actual_hash = actual.as<hash>();
    ASSERT_EQ(actual_hash.size(), expected.size());
}

auto assert_error_object(const error& expected, const object& actual, std::string_view input) -> void
{
    ASSERT_TRUE(actual.is<error>()) << input << " got " << actual.type_name() << " with "
                                    << std::to_string(actual.value) << " instead ";
    auto actual_error = actual.as<error>();
    ASSERT_EQ(actual_error.message, expected.message);
}

template<typename... T>
struct vt
{
    std::string_view input;
    std::variant<T...> expected;
};

template<typename... T>
auto assert_expected_object(const std::variant<T...>& expected, const object& actual, std::string_view input) -> void
{
    std::visit(
        overloaded {
            [&](const int64_t exp) { assert_integer_object(exp, actual, input); },
            [&](const bool exp) { assert_bool_object(exp, actual, input); },
            [&](const nil_type) { ASSERT_TRUE(actual.is_nil()); },
            [&](const std::string& exp) { assert_string_object(exp, actual, input); },
            [&](const error& exp) { assert_error_object(exp, actual, input); },
            [&](const std::vector<int>& exp) { assert_array_object(exp, actual, input); },
            [&](const hash& exp) { assert_hash_object(exp, actual, input); },
        },
        expected);
}

template<size_t N, typename... Expecteds>
auto run(std::array<vt<Expecteds...>, N> tests)
{
    for (const auto& [input, expected] : tests) {
        auto [prgrm, _] = assert_program(input);
        auto cmplr = compiler::create();
        cmplr.compile(prgrm);
        auto mchn = vm::create(cmplr.byte_code());
        mchn.run();

        auto top = mchn.last_popped();
        assert_expected_object(expected, top, input);
    }
}

// NOLINTBEGIN(*-magic-numbers)
TEST(vm, integerArithmetics)
{
    std::array tests {
        vt<int64_t> {"1", 1},
        vt<int64_t> {"2", 2},
        vt<int64_t> {"1 + 2", 3},
        vt<int64_t> {"1 - 2", -1},
        vt<int64_t> {"1 * 2", 2},

        vt<int64_t> {"4 / 2", 2},
        vt<int64_t> {"50 / 2 * 2 + 10 - 5", 55},
        vt<int64_t> {"5 + 5 + 5 + 5 - 10", 10},
        vt<int64_t> {"2 * 2 * 2 * 2 * 2", 32},
        vt<int64_t> {"5 * 2 + 10", 20},
        vt<int64_t> {"5 + 2 * 10", 25},
        vt<int64_t> {"5 * (2 + 10)", 60},
        vt<int64_t> {"-5", -5},
        vt<int64_t> {"-10", -10},
        vt<int64_t> {"-50 + 100 + -50", 0},
        vt<int64_t> {"(5 + 10 * 2 + 15 / 3) * 2 + -10", 50},
    };
    run(tests);
}

TEST(vm, booleanExpressions)
{
    std::array tests {
        vt<bool> {"true", true},
        vt<bool> {"false", false},
        vt<bool> {"1 < 2", true},
        vt<bool> {"1 > 2", false},
        vt<bool> {"1 < 1", false},
        vt<bool> {"1 > 1", false},
        vt<bool> {"1 == 1", true},
        vt<bool> {"1 != 1", false},
        vt<bool> {"1 == 2", false},
        vt<bool> {"1 != 2", true},
        vt<bool> {"true == true", true},
        vt<bool> {"false == false", true},
        vt<bool> {"true == false", false},
        vt<bool> {"true != false", true},
        vt<bool> {"false != true", true},
        vt<bool> {"(1 < 2) == true", true},
        vt<bool> {"(1 < 2) == false", false},
        vt<bool> {"(1 > 2) == true", false},
        vt<bool> {"(1 > 2) == false", true},
        vt<bool> {"!true", false},
        vt<bool> {"!false", true},
        vt<bool> {"!5", false},
        vt<bool> {"!!true", true},
        vt<bool> {"!!false", false},
        vt<bool> {"!!5", true},
        vt<bool> {"!(if (false) { 5; })", true},
    };
    run(tests);
}

TEST(vm, conditionals)
{
    std::array tests {
        vt<int64_t, nil_type> {"if (true) { 10 }", 10},
        vt<int64_t, nil_type> {"if (true) { 10 } else { 20 }", 10},
        vt<int64_t, nil_type> {"if (false) { 10 } else { 20 } ", 20},
        vt<int64_t, nil_type> {"if (1) { 10 }", 10},
        vt<int64_t, nil_type> {"if (1 < 2) { 10 }", 10},
        vt<int64_t, nil_type> {"if (1 < 2) { 10 } else { 20 }", 10},
        vt<int64_t, nil_type> {"if (1 > 2) { 10 } else { 20 }", 20},
        vt<int64_t, nil_type> {"if (1 > 2) { 10 }", nil_type {}},
        vt<int64_t, nil_type> {"if (false) { 10 }", nil_type {}},
        vt<int64_t, nil_type> {"if ((if (false) { 10 })) { 10 } else { 20 }", 20},
    };
    run(tests);
}

TEST(vm, globalLetStatemets)
{
    std::array tests {
        vt<int64_t> {"let one = 1; one", 1},
        vt<int64_t> {"let one = 1; let two = 2; one + two", 3},
        vt<int64_t> {"let one = 1; let two = one + one; one + two", 3},
    };
    run(tests);
}

TEST(vm, stringExpression)
{
    std::array tests {
        vt<std::string> {R"("monkey")", "monkey"},
        vt<std::string> {R"("mon" + "key")", "monkey"},
        vt<std::string> {R"("mon" + "key" + "banana")", "monkeybanana"},

    };
    run(tests);
}

TEST(vm, arrayLiterals)
{
    std::array tests {
        vt<std::vector<int>> {
            "[]",
            {},
        },
        vt<std::vector<int>> {
            "[1, 2, 3]",
            {std::vector<int> {1, 2, 3}},
        },
        vt<std::vector<int>> {
            "[1+ 2, 3 * 4, 5 + 6]",
            {std::vector<int> {3, 12, 11}},
        },
    };
    run(tests);
}

TEST(vm, hashLiterals)
{
    std::array tests {
        vt<hash> {
            "{}",
            {},
        },
        vt<hash> {
            "{1: 2, 3: 4}",
            {hash {{1, 2}, {3, 4}}},
        },
        vt<hash> {
            "{1 + 1: 2 * 2, 3 + 3: 4 * 4}",
            {hash {{2, 4}, {6, 16}}},

        },
    };
    run(tests);
}

TEST(vm, indexExpressions)
{
    std::array tests {
        vt<int64_t, nil_type> {"[1, 2, 3][1]", 2},
        vt<int64_t, nil_type> {"[1, 2, 3][0 + 2]", 3},
        vt<int64_t, nil_type> {"[[1, 1, 1]][0][0]", 1},
        vt<int64_t, nil_type> {"[][0]", nil_type {}},
        vt<int64_t, nil_type> {"[1, 2, 3][99]", nil_type {}},
        vt<int64_t, nil_type> {"[1][-1]", nil_type {}},
        vt<int64_t, nil_type> {"{1: 1, 2: 2}[1]", 1},
        vt<int64_t, nil_type> {"{1: 1, 2: 2}[2]", 2},
        vt<int64_t, nil_type> {"{1: 1}[0]", nil_type {}},
        vt<int64_t, nil_type> {"{}[0]", nil_type {}},
    };
    run(tests);
}

TEST(vm, callFunctionsWithoutArgs)
{
    std::array tests {
        vt<int64_t> {
            R"(let fivePlusTen = fn() { 5 + 10; };
               fivePlusTen();)",
            15,
        },
        vt<int64_t> {
            R"(let one = fn() { 1; };
               let two = fn() { 2; };
               one() + two())",
            3,
        },
        vt<int64_t> {
            R"(let a = fn() { 1 };
               let b = fn() { a() + 1 };
               let c = fn() { b() + 1 };
               c();)",
            3,
        },
    };
    run(tests);
}

TEST(vm, callFunctionsWithReturnStatements)
{
    std::array tests {
        vt<int64_t> {
            R"(let earlyExit = fn() { return 99; 100; };
               earlyExit();)",
            99,
        },
        vt<int64_t> {
            R"(let earlyExit = fn() { return 99; return 100; };
               earlyExit();)",
            99,
        },
    };
    run(tests);
}

TEST(vm, callFunctionsWithNoReturnValue)
{
    std::array tests {
        vt<nil_type> {
            R"(let noReturn = fn() { };
               noReturn();)",
            nilv,
        },
        vt<nil_type> {
            R"(let noReturn = fn() { };
               let noReturnTwo = fn() { noReturn(); };
               noReturn();
               noReturnTwo();)",
            nilv,
        },
    };
    run(tests);
}

TEST(vm, callFirstClassFunctions)
{
    std::array tests {
        vt<int64_t> {
            R"(let returnsOne = fn() { 1; };
               let returnsOneReturner = fn() { returnsOne; };
               returnsOneReturner()();)",
            1,
        },
        vt<int64_t> {
            R"(
        let globalSeed = 50;
        let minusOne = fn() {
            let num = 1;
            globalSeed - num;
        }
        let minusTwo = fn() {
            let num = 2;
            globalSeed - num;
        }
        minusOne() + minusOne();)",
            98,
        },
    };
    run(tests);
}

TEST(vm, callFunctionsWithBindings)
{
    std::array tests {
        vt<int64_t> {
            R"(
            let one = fn() { let one = 1; one };
            one();
            )",
            1,
        },
        vt<int64_t> {
            R"(
        let oneAndTwo = fn() { let one = 1; let two = 2; one + two; };
        oneAndTwo();
            )",
            3,
        },
        vt<int64_t> {
            R"(
        let oneAndTwo = fn() { let one = 1; let two = 2; one + two; };
        let threeAndFour = fn() { let three = 3; let four = 4; three + four; };
        oneAndTwo() + threeAndFour();
            )",
            10,
        },
        vt<int64_t> {
            R"(
        let firstFoobar = fn() { let foobar = 50; foobar; };
        let secondFoobar = fn() { let foobar = 100; foobar; };
        firstFoobar() + secondFoobar();
            )",
            150,
        },
        vt<int64_t> {
            R"(
        let globalSeed = 50;
        let minusOne = fn() {
            let num = 1;
            globalSeed - num;
        }
        let minusTwo = fn() {
            let num = 2;
            globalSeed - num;
        }
        minusOne() + minusTwo();
            )",
            97,
        },
    };
    run(tests);
}

TEST(vm, callFunctionsWithArgumentsAndBindings)
{
    std::array tests {
        vt<int64_t> {
            R"(
        let identity = fn(a) { a; };
        identity(4);
            )",
            4,
        },
        vt<int64_t> {
            R"(
        let sum = fn(a, b) { a + b; };
        sum(1, 2);
            )",
            3,
        },
        vt<int64_t> {
            R"(
         let sum = fn(a, b) {
            let c = a + b;
            c;
        };
        sum(1, 2);
            )",
            3,
        },
        vt<int64_t> {
            R"(
        let sum = fn(a, b) {
            let c = a + b;
            c;
        };
        sum(1, 2) + sum(3, 4);
            )",
            10,
        },
        vt<int64_t> {
            R"(
         let sum = fn(a, b) {
            let c = a + b;
            c;
        };
        let outer = fn() {
            sum(1, 2) + sum(3, 4);
        };
        outer();
            )",
            10,
        },
        vt<int64_t> {
            R"(
        let globalNum = 10;

        let sum = fn(a, b) {
            let c = a + b;
            c + globalNum;
        };

        let outer = fn() {
            sum(1, 2) + sum(3, 4) + globalNum;
        };

        outer() + globalNum;
            )",
            50,
        },
    };
    run(tests);
}

TEST(vm, callFunctionsWithWrongArgument)
{
    std::array tests {
        vt<std::string> {
            R"(fn() { 1; }(1);)",
            "wrong number of arguments: want=0, got=1",
        },
        vt<std::string> {
            R"(fn(a) { a; }();)",
            "wrong number of arguments: want=1, got=0",

        },
        vt<std::string> {
            R"(fn(a, b) { a + b; }(1);)",
            "wrong number of arguments: want=2, got=1",
        },
    };
    for (const auto& [input, expected] : tests) {
        auto [prgrm, _] = assert_program(input);
        auto cmplr = compiler::create();
        cmplr.compile(prgrm);
        auto mchn = vm::create(cmplr.byte_code());
        try {
            mchn.run();
        } catch (const std::exception& e) {
            ASSERT_STREQ(e.what(), std::get<std::string>(expected).c_str());
            continue;
        }
        FAIL() << "Did not throw";
    }
}

TEST(vm, callBuiltins)
{
    std::array tests {
        vt<int64_t> {R"(len(""))", 0}, vt<int64_t> {R"(len("four"))", 4}, vt<int64_t> {R"(len("hello world"))", 11},
        /*
                vt<int64_t, error, nil_type, std::vector<int>> {
                    R"(len(1))",
                    error {
                        "argument to `len` not supported, got INTEGER",
                    },
                },
                vt<int64_t, error, nil_type, std::vector<int64_t>> {
                    R"(len("one", "two"))",
                    error {
                        "wrong number of arguments. got=2, want=1",
                    },
                },
                vt<int64_t, error, nil_type, std::vector<int>> {R"(len([1, 2, 3]))", 3},
                vt<int64_t, error, nil_type, std::vector<int>> {R"(len([]))", 0},
                vt<int64_t, error, nil_type, std::vector<int>> {R"(puts("hello", "world!"))", nilv},
                vt<int64_t, error, nil_type, std::vector<int>> {R"(first([1, 2, 3]))", 1},
                vt<int64_t, error, nil_type, std::vector<int>> {R"(first([]))", nilv},
                vt<int64_t, error, nil_type, std::vector<int>> {
                    R"(first(1))",
                    error {
                        "argument to `first` must be ARRAY, got INTEGER",
                    },
                },
                vt<int64_t, error, nil_type, std::vector<int>> {R"(last([1, 2, 3]))", 3},
                vt<int64_t, error, nil_type, std::vector<int>> {R"(last([]))", nilv},
                {
                    R"(last(1))",
                    error {
                        "argument to `last` must be ARRAY, got INTEGER",
                    },
                },
                vt<int64_t, error, nil_type, std::vector<int>> {R"(rest([1, 2, 3]))", maker<int>({2, 3})},
                vt<int64_t, error, nil_type, std::vector<int>> {R"(rest([]))", nilv},
                vt<int64_t, error, nil_type, std::vector<int>> {R"(push([], 1))", maker<int>({1})},
                vt<int64_t, error, nil_type, std::vector<int>> {
                    R"(push(1, 1))",
                    error {
                        "argument to `push` must be ARRAY, got INTEGER",
                    },
                },*/
    };
    run(tests);
}

// NOLINTEND(*-magic-numbers)
