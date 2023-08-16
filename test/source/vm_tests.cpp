#include <cstdint>
#include <string_view>

#include <compiler/vm.hpp>
#include <gtest/gtest.h>

#include "compiler.hpp"
#include "object.hpp"
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
    ASSERT_TRUE(actual.is<integer_value>())
        << input << " got " << actual.type_name() << " with " << std::to_string(actual.value) << " instead ";
    auto actual_int = actual.as<integer_value>();
    ASSERT_EQ(actual_int, expected);
}

template<typename... T>
struct vm_test
{
    std::string_view input;
    std::variant<T...> expected;
};

template<typename... T>
auto assert_expected_object(const std::variant<T...>& expected, const object& actual, std::string_view input) -> void
{
    std::visit(
        overloaded {
            [&](const int64_t expected) { assert_integer_object(expected, actual, input); },
            [&](const bool expected) { assert_bool_object(expected, actual, input); },
        },
        expected);
}

template<size_t N, typename... Expecteds>
auto run_vm_test(std::array<vm_test<Expecteds...>, N> tests)
{
    for (const auto& [input, expected] : tests) {
        auto [prgrm, _] = assert_program(input);
        compiler piler;
        piler.compile(prgrm);
        auto bytecode = piler.code();
        vm vm {
            .consts = bytecode.consts,
            .code = bytecode.code,
        };
        vm.run();

        auto top = vm.last_popped();
        assert_expected_object(expected, top, input);
    }
}

// NOLINTBEGIN(*-magic-numbers)
TEST(vm, integerArithmetics)
{
    std::array tests {
        vm_test<int64_t> {"1", 1},
        vm_test<int64_t> {"2", 2},
        vm_test<int64_t> {"1 + 2", 3},
        vm_test<int64_t> {"1 - 2", -1},
        vm_test<int64_t> {"1 * 2", 2},

        vm_test<int64_t> {"4 / 2", 2},
        vm_test<int64_t> {"50 / 2 * 2 + 10 - 5", 55},
        vm_test<int64_t> {"5 + 5 + 5 + 5 - 10", 10},
        vm_test<int64_t> {"2 * 2 * 2 * 2 * 2", 32},
        vm_test<int64_t> {"5 * 2 + 10", 20},
        vm_test<int64_t> {"5 + 2 * 10", 25},
        vm_test<int64_t> {"5 * (2 + 10)", 60},
    };
    run_vm_test(tests);
}

TEST(vm, booleanExpressions)
{
    std::array tests {
        vm_test<bool> {"true", true},
        vm_test<bool> {"false", false},
        vm_test<bool> {"1 < 2", true},
        vm_test<bool> {"1 > 2", false},
        vm_test<bool> {"1 < 1", false},
        vm_test<bool> {"1 > 1", false},
        vm_test<bool> {"1 == 1", true},
        vm_test<bool> {"1 != 1", false},
        vm_test<bool> {"1 == 2", false},
        vm_test<bool> {"1 != 2", true},
        vm_test<bool> {"true == true", true},
        vm_test<bool> {"false == false", true},
        vm_test<bool> {"true == false", false},
        vm_test<bool> {"true != false", true},
        vm_test<bool> {"false != true", true},
        vm_test<bool> {"(1 < 2) == true", true},
        vm_test<bool> {"(1 < 2) == false", false},
        vm_test<bool> {"(1 > 2) == true", false},
        vm_test<bool> {"(1 > 2) == false", true},
    };
    run_vm_test(tests);
}
// NOLINTEND(*-magic-numbers)
