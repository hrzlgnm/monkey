#include <array>
#include <cstdint>
#include <stdexcept>

#include "vm.hpp"

#include <ast/builtin_function_expression.hpp>
#include <code/code.hpp>
#include <compiler/compiler.hpp>
#include <doctest/doctest.h>
#include <eval/object.hpp>
#include <fmt/core.h>
#include <parser/parser.hpp>

static const object tru {true};
static const object fals {false};

vm::vm(frames&& frames, constants_ptr&& consts, constants_ptr globals)
    : m_constans {std::move(consts)}
    , m_globals {std::move(globals)}
    , m_frames {std::move(frames)}
{
}

auto vm::run() -> void
{
    for (; current_frame().ip < static_cast<ssize_t>(current_frame().cl.fn.instrs.size()); current_frame().ip++) {
        const auto instr_ptr = static_cast<size_t>(current_frame().ip);
        auto& code = current_frame().cl.fn.instrs;
        const auto op_code = static_cast<opcodes>(code.at(instr_ptr));
        switch (op_code) {
            case opcodes::constant: {
                auto const_idx = read_uint16_big_endian(code, instr_ptr + 1UL);
                current_frame().ip += 2;
                push(m_constans->at(const_idx));
            } break;
            case opcodes::sub:  // fallthrough
            case opcodes::mul:  // fallthrough
            case opcodes::div:  // fallthrough
            case opcodes::add:
                exec_binary_op(op_code);
                break;
            case opcodes::equal:  // fallthrough
            case opcodes::not_equal:  // fallthrough
            case opcodes::greater_than:
                exec_cmp(op_code);
                break;
            case opcodes::pop:
                pop();
                break;
            case opcodes::tru:
                push(tru);
                break;
            case opcodes::fals:
                push(fals);
                break;
            case opcodes::bang:
                exec_bang();
                break;
            case opcodes::minus:
                exec_minus();
                break;
            case opcodes::jump: {
                auto pos = read_uint16_big_endian(code, instr_ptr + 1UL);
                current_frame().ip = pos - 1;
            } break;
            case opcodes::jump_not_truthy: {
                auto pos = read_uint16_big_endian(code, instr_ptr + 1UL);
                current_frame().ip += 2;
                auto condition = pop();
                if (!condition.is_truthy()) {
                    current_frame().ip = pos - 1;
                }

            } break;
            case opcodes::null:
                push(nil);
                break;
            case opcodes::set_global: {
                auto global_index = read_uint16_big_endian(code, instr_ptr + 1UL);
                current_frame().ip += 2;
                m_globals->at(global_index) = pop();
            } break;
            case opcodes::get_global: {
                auto global_index = read_uint16_big_endian(code, instr_ptr + 1UL);
                current_frame().ip += 2;
                push(m_globals->at(global_index));
            } break;
            case opcodes::array: {
                auto num_elements = read_uint16_big_endian(code, instr_ptr + 1UL);
                current_frame().ip += 2;
                auto arr = build_array(m_sp - num_elements, m_sp);
                m_sp -= num_elements;
                push(arr);
            } break;
            case opcodes::hash: {
                auto num_elements = read_uint16_big_endian(code, instr_ptr + 1UL);
                current_frame().ip += 2;

                auto hsh = build_hash(m_sp - num_elements, m_sp);
                m_sp -= num_elements;
                push(hsh);
            } break;
            case opcodes::index: {
                auto index = pop();
                auto left = pop();
                exec_index(std::move(left), std::move(index));
            } break;
            case opcodes::call: {
                auto num_args = code.at(instr_ptr + 1UL);
                current_frame().ip += 1;
                exec_call(num_args);
            } break;
            case opcodes::return_value: {
                auto return_value = pop();
                auto frame = pop_frame();
                m_sp = static_cast<size_t>(frame.base_ptr) - 1;
                push(return_value);
            } break;
            case opcodes::ret: {
                auto frame = pop_frame();
                m_sp = static_cast<size_t>(frame.base_ptr) - 1;
                push(nil);
            } break;
            case opcodes::set_local: {
                auto local_index = code.at(instr_ptr + 1UL);
                current_frame().ip += 1;
                auto& frame = current_frame();
                m_stack[static_cast<size_t>(frame.base_ptr) + local_index] = pop();
            } break;
            case opcodes::get_local: {
                auto local_index = code.at(instr_ptr + 1UL);
                current_frame().ip += 1;
                auto& frame = current_frame();
                push(m_stack[static_cast<size_t>(frame.base_ptr) + local_index]);
            } break;
            case opcodes::get_builtin: {
                auto builtin_index = code.at(instr_ptr + 1UL);
                current_frame().ip += 1;
                const auto& builtin = builtin_function_expression::builtins[builtin_index];
                push({&builtin});
            } break;
            case opcodes::closure: {
                auto const_idx = read_uint16_big_endian(code, instr_ptr + 1UL);
                auto num_free = code.at(instr_ptr + 3UL);
                current_frame().ip += 3;
                push_closure(const_idx, num_free);
            } break;
            case opcodes::get_free: {
                auto free_index = code.at(instr_ptr + 1UL);
                current_frame().ip += 1;
                auto current_closure = current_frame().cl;
                push(current_closure.free[free_index]);
            } break;
            case opcodes::current_closure: {
                push({current_frame().cl});
            } break;
            default:
                throw std::runtime_error(fmt::format("opcode {} not implemented yet", op_code));
        }
    }
}

auto vm::push(const object& obj) -> void
{
    if (m_sp >= stack_size) {
        throw std::runtime_error("stack overflow");
    }
    m_stack[m_sp] = obj;
    m_sp++;
}

auto vm::pop() -> object
{
    if (m_sp == 0) {
        throw std::runtime_error("stack empty");
    }
    auto result = m_stack[m_sp - 1];
    m_sp--;
    return result;
}

auto vm::last_popped() const -> object
{
    return m_stack[m_sp];
}

auto vm::exec_binary_op(opcodes opcode) -> void
{
    auto right = pop();
    auto left = pop();
    if (left.is<integer_type>() && right.is<integer_type>()) {
        auto left_value = left.as<integer_type>();
        auto right_value = right.as<integer_type>();
        switch (opcode) {
            case opcodes::sub:
                push({left_value - right_value});
                break;
            case opcodes::mul:
                push({left_value * right_value});
                break;
            case opcodes::div:
                push({left_value / right_value});
                break;
            case opcodes::add:
                push({left_value + right_value});
                break;
            default:
                throw std::runtime_error(fmt::format("unknown integer operator"));
        }
        return;
    }
    if (left.is<string_type>() && right.is<string_type>()) {
        const auto& left_value = left.as<string_type>();
        const auto& right_value = right.as<string_type>();
        switch (opcode) {
            case opcodes::add:
                push({left_value + right_value});
                break;
            default:
                throw std::runtime_error(fmt::format("unknown string operator"));
        }
        return;
    }
    throw std::runtime_error(
        fmt::format("unsupported types for binary operation: {} {}", left.type_name(), right.type_name()));
}

auto exec_int_cmp(opcodes opcode, integer_type lhs, integer_type rhs) -> bool
{
    using enum opcodes;
    switch (opcode) {
        case equal:
            return lhs == rhs;
        case not_equal:
            return lhs != rhs;
        case greater_than:
            return lhs > rhs;
        default:
            throw std::runtime_error(fmt::format("unknown operator {}", opcode));
    }
}

auto vm::exec_cmp(opcodes opcode) -> void
{
    auto right = pop();
    auto left = pop();
    if (left.is<integer_type>() && right.is<integer_type>()) {
        push({exec_int_cmp(opcode, left.as<integer_type>(), right.as<integer_type>())});
        return;
    }

    using enum opcodes;
    switch (opcode) {
        case equal:
            return push({left == right});
        case not_equal:
            return push({left != right});
        default:
            throw std::runtime_error(
                fmt::format("unknown operator {} ({} {})", opcode, left.type_name(), right.type_name()));
    }
}

auto vm::exec_bang() -> void
{
    auto operand = pop();
    if (operand.is<bool>()) {
        return push({!operand.as<bool>()});
    }
    if (operand.is_nil()) {
        return push(tru);
    }
    push(fals);
}

auto vm::exec_minus() -> void
{
    auto operand = pop();
    if (!operand.is<integer_type>()) {
        throw std::runtime_error(fmt::format("unsupported type for negation {}", operand.type_name()));
    }
    push({-operand.as<integer_type>()});
}

auto vm::build_array(size_t start, size_t end) const -> object
{
    array arr;
    for (auto idx = start; idx < end; idx++) {
        arr.push_back(m_stack.at(idx));
    }
    return {arr};
}

auto vm::build_hash(size_t start, size_t end) const -> object
{
    hash hsh;
    for (auto idx = start; idx < end; idx += 2) {
        auto key = m_stack[idx];
        auto val = m_stack[idx + 1];
        hsh[key.hash_key()] = val;
    }
    return {hsh};
}

auto exec_hash(const hash& hsh, const hash_key_type& key) -> object
{
    if (!hsh.contains(key)) {
        return nil;
    }
    return unwrap(hsh.at(key));
}

auto vm::exec_index(object&& left, object&& index) -> void
{
    std::visit(
        overloaded {[&](const array& arr, int64_t index)
                    {
                        auto max = static_cast<int64_t>(arr.size() - 1);
                        if (index < 0 || index > max) {
                            return push(nil);
                        }
                        return push(arr.at(static_cast<size_t>(index)));
                    },
                    [&](const hash& hsh, bool index) { push(exec_hash(hsh, {index})); },
                    [&](const hash& hsh, int64_t index) { push(exec_hash(hsh, {index})); },
                    [&](const hash& hsh, const std::string& index) { push(exec_hash(hsh, {index})); },
                    [&](const auto& lft, const auto& idx)
                    {
                        throw std::runtime_error(fmt::format(
                            "invalid index operation: {}:[{}]", object {lft}.type_name(), object {idx}.type_name()));
                    }},
        left.value,
        index.value);
}

auto vm::exec_call(size_t num_args) -> void
{
    auto callee = m_stack[m_sp - 1 - num_args];
    std::visit(
        overloaded {[&](const closure& closure)
                    {
                        if (num_args != closure.fn.num_arguments) {
                            throw std::runtime_error(fmt::format(
                                "wrong number of arguments: want={}, got={}", closure.fn.num_arguments, num_args));
                        }
                        frame frm {closure, -1, static_cast<ssize_t>(m_sp - num_args)};
                        m_sp = static_cast<size_t>(frm.base_ptr) + closure.fn.num_locals;
                        push_frame(std::move(frm));
                    },
                    [&](const builtin_function_expression* builtin)
                    {
                        array args;
                        for (auto idx = m_sp - num_args; idx < m_sp; idx++) {
                            args.push_back(m_stack[idx]);
                        }
                        auto result = builtin->body(std::move(args));
                        push(result);
                    },
                    [](const auto& /*other*/) { throw std::runtime_error("calling non-closure and non-builtin"); }},
        callee.value);
}

auto vm::current_frame() -> frame&
{
    return m_frames[m_frame_index - 1];
}

auto vm::push_frame(frame&& frm) -> void
{
    m_frames[m_frame_index] = std::move(frm);
    m_frame_index++;
}

auto vm::pop_frame() -> frame
{
    m_frame_index--;
    return m_frames[m_frame_index];
}

auto vm::push_closure(uint16_t const_idx, uint8_t num_free) -> void
{
    auto constant = m_constans->at(const_idx);
    if (!constant.is<compiled_function>()) {
        throw std::runtime_error("not a function");
    }
    array free;
    for (auto i = 0UL; i < num_free; i++) {
        free.push_back(m_stack.at(m_sp - num_free + i));
    }
    m_sp -= num_free;
    push({closure {.fn = constant.as<compiled_function>(), .free = free}});
}

namespace
{
// NOLINTBEGIN(*)

TEST_SUITE_BEGIN("vm");

template<typename T>
auto maker(std::initializer_list<T> list) -> std::vector<T>
{
    return std::vector<T> {list};
}

auto assert_no_parse_errors(const parser& prsr) -> bool
{
    INFO("expected no errors, got:", fmt::format("{}", fmt::join(prsr.errors(), ", ")));
    CHECK(prsr.errors().empty());
    return !prsr.errors().empty();
}

using parsed_program = std::pair<program_ptr, parser>;

auto assert_program(std::string_view input) -> parsed_program
{
    auto prsr = parser {lexer {input}};
    auto prgrm = prsr.parse_program();
    if (assert_no_parse_errors(prsr)) {
        INFO("while parsing: `", input, "`");
    };
    return {std::move(prgrm), std::move(prsr)};
}

auto assert_bool_object(bool expected, const object& actual, std::string_view input) -> void
{
    INFO(input, " got ", actual.type_name(), " with ", std::to_string(actual.value), " instead");
    REQUIRE(actual.is<bool>());
    auto actual_bool = actual.as<bool>();
    REQUIRE_EQ(actual_bool, expected);
}

auto assert_integer_object(int64_t expected, const object& actual, std::string_view input) -> void
{
    INFO(input, " got ", actual.type_name(), " with ", std::to_string(actual.value), " instead");
    REQUIRE(actual.is<integer_type>());
    auto actual_int = actual.as<integer_type>();
    REQUIRE_EQ(actual_int, expected);
}

auto assert_string_object(const std::string& expected, const object& actual, std::string_view input) -> void
{
    INFO(input, " got ", actual.type_name(), " with ", std::to_string(actual.value), " instead");
    REQUIRE(actual.is<string_type>());
    auto actual_str = actual.as<string_type>();
    REQUIRE_EQ(actual_str, expected);
}

auto assert_array_object(const std::vector<int>& expected, const object& actual, std::string_view input) -> void
{
    INFO(input, " got ", actual.type_name(), " with ", std::to_string(actual.value), " instead");
    REQUIRE(actual.is<array>());
    auto actual_arr = actual.as<array>();
    REQUIRE_EQ(actual_arr.size(), expected.size());
    for (auto idx = 0UL; const auto& elem : expected) {
        REQUIRE_EQ(elem, actual_arr.at(idx).as<integer_type>());
        ++idx;
    }
}

auto assert_hash_object(const hash& expected, const object& actual, std::string_view input) -> void
{
    INFO(input, " got ", actual.type_name(), " with ", std::to_string(actual.value), " instead");
    REQUIRE(actual.is<hash>());
    auto actual_hash = actual.as<hash>();
    REQUIRE_EQ(actual_hash.size(), expected.size());
}

auto assert_error_object(const error& expected, const object& actual, std::string_view input) -> void
{
    INFO(input, " got ", actual.type_name(), " with ", std::to_string(actual.value), " instead");
    REQUIRE(actual.is<error>());
    auto actual_error = actual.as<error>();
    REQUIRE_EQ(actual_error.message, expected.message);
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
            [&](const nil_type) { REQUIRE(actual.is_nil()); },
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
        auto byte_code = cmplr.byte_code();
        auto mchn = vm::create(std::move(byte_code));
        mchn.run();

        auto top = mchn.last_popped();
        assert_expected_object(expected, top, input);
    }
}

TEST_CASE("integerArithmetics")
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

TEST_CASE("booleanExpressions")
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

TEST_CASE("conditionals")
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

TEST_CASE("globalLetStatemets")
{
    std::array tests {
        vt<int64_t> {"let one = 1; one", 1},
        vt<int64_t> {"let one = 1; let two = 2; one + two", 3},
        vt<int64_t> {"let one = 1; let two = one + one; one + two", 3},
    };
    run(tests);
}

TEST_CASE("stringExpression")
{
    std::array tests {
        vt<std::string> {R"("monkey")", "monkey"},
        vt<std::string> {R"("mon" + "key")", "monkey"},
        vt<std::string> {R"("mon" + "key" + "banana")", "monkeybanana"},

    };
    run(tests);
}

TEST_CASE("arrayLiterals")
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

TEST_CASE("hashLiterals")
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

TEST_CASE("indexExpressions")
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

TEST_CASE("callFunctionsWithoutArgs")
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

TEST_CASE("callFunctionsWithReturnStatements")
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

TEST_CASE("callFunctionsWithNoReturnValue")
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

TEST_CASE("callFirstClassFunctions")
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

TEST_CASE("callFunctionsWithBindings")
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

TEST_CASE("callFunctionsWithArgumentsAndBindings")
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

TEST_CASE("callFunctionsWithWrongArgument")
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
        CHECK_THROWS_WITH(mchn.run(), std::get<std::string>(expected).c_str());
    }
}

TEST_CASE("callBuiltins")
{
    std::array tests {
        vt<int64_t, nil_type, std::vector<int>> {R"(len(""))", 0},
        vt<int64_t, nil_type, std::vector<int>> {R"(len("four"))", 4},
        vt<int64_t, nil_type, std::vector<int>> {R"(len("hello world"))", 11},
        vt<int64_t, nil_type, std::vector<int>> {R"(len([1, 2, 3]))", 3},
        vt<int64_t, nil_type, std::vector<int>> {R"(len([]))", 0},
        vt<int64_t, nil_type, std::vector<int>> {R"(puts("hello", "world!"))", nilv},
        vt<int64_t, nil_type, std::vector<int>> {R"(first([1, 2, 3]))", 1},
        vt<int64_t, nil_type, std::vector<int>> {R"(first([]))", nilv},
        vt<int64_t, nil_type, std::vector<int>> {R"(last([]))", nilv},
        vt<int64_t, nil_type, std::vector<int>> {R"(rest([1, 2, 3]))", maker<int>({2, 3})},
        vt<int64_t, nil_type, std::vector<int>> {R"(rest([]))", nilv},
        vt<int64_t, nil_type, std::vector<int>> {R"(push([], 1))", maker<int>({1})},
        vt<int64_t, nil_type, std::vector<int>> {R"(last([1, 2, 3]))", 3},
    };
    std::array errortests {
        vt<error> {
            R"(len(1))",
            error {
                "argument of type integer to len() is not supported",
            },
        },
        vt<error> {
            R"(len("one", "two"))",
            error {
                "wrong number of arguments to len(): expected=1, got=2",
            },
        },
        vt<error> {
            R"(first(1))",
            error {
                "argument of type integer to first() is not supported",
            },
        },
        vt<error> {
            R"(last(1))",
            error {
                "argument of type integer to last() is not supported",
            },
        },
        vt<error> {
            R"(push(1, 1))",
            error {
                "argument of type integer and integer to push() are not supported",
            },
        },
    };
    run(tests);
    run(errortests);
}

TEST_CASE("closures")
{
    std::array tests {
        vt<int64_t> {
            R"(
        let newClosure = fn(a) {
            fn() { a; };
        };
        let closure = newClosure(99);
        closure();
            )",
            99,
        },
    };
    run(tests);
}

TEST_CASE("recursiveFunction")
{
    using enum opcodes;
    std::array tests {
        vt<int64_t> {
            R"(
        let countDown = fn(x) {
            if (x == 0) {
                return 0;
            } else {
                countDown(x - 1);
            }
        };
        countDown(1);
            )",
            0,
        },
        vt<int64_t> {
            R"(
        let countDown = fn(x) {
            if (x == 0) {
                return 0;
            } else {
                countDown(x - 1);
            }
        };
        let wrapper = fn() {
            countDown(1);
        };
        wrapper();
            )",
            0,
        },
        vt<int64_t> {
            R"(
        let wrapper = fn() {
            let countDown = fn(x) {
                if (x == 0) {
                    return 0;
                } else {
                    countDown(x - 1);
                }
            };
            countDown(1);
        };
        wrapper();
        )",
            0,
        },
    };
    run(tests);
}

TEST_CASE("recuriveFibonnacci")
{
    std::array tests {
        vt<int64_t> {
            R"(
        let fibonacci = fn(x) {
            if (x == 0) {
                return 0;
            } else {
                if (x == 1) {
                    return 1;
                } else {
                    fibonacci(x - 1) + fibonacci(x - 2);
                }
            }
        };
        fibonacci(15);)",
            610},
    };
    run(tests);
}

TEST_SUITE_END();
}  // namespace

// NOLINTEND(*)
