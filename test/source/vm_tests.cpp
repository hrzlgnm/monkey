#include <cstdint>

#include <compiler/vm.hpp>
#include <gtest/gtest.h>

#include "compiler.hpp"
#include "object.hpp"
#include "testutils.hpp"

auto assert_bool_object(bool expected, const object& actual) -> void
{
    ASSERT_TRUE(actual.is<bool>()) << "got " << actual.type_name() << " instead";
    auto actual_int = actual.as<bool>();
    ASSERT_EQ(actual_int, expected);
}

auto assert_integer_object(int64_t expected, const object& actual) -> void
{
    ASSERT_TRUE(actual.is<integer_value>()) << "got " << actual.type_name() << " instead";
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
auto assert_expected_object(const std::variant<T...>& expected, const object& actual) -> void
{
    std::visit(
        overloaded {
            [&actual](const int64_t expected) { assert_integer_object(expected, actual); },
            [&actual](bool expected) { assert_bool_object(expected, actual); },
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
        assert_expected_object(expected, top);
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
    };
    run_vm_test(tests);
}
// NOLINTEND(*-magic-numbers)
