#include <cstdint>

#include <compiler/vm.hpp>
#include <gtest/gtest.h>

#include "compiler.hpp"
#include "object.hpp"
#include "testutils.hpp"

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
    std::visit(overloaded {[&actual](const int64_t expected) { assert_integer_object(expected, actual); }}, expected);
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

        auto top = vm.stack_top();
        assert_expected_object(expected, top);
    }
}

TEST(vm, integerArithmetics)
{
    std::array tests {vm_test<int64_t> {"1", 1}, vm_test<int64_t> {"2", 2}, vm_test<int64_t> {"1 + 2", 3}};
    run_vm_test(tests);
}
