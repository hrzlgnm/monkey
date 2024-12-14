#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

#include "vm.hpp"

#include <ast/builtin_function_expression.hpp>
#include <code/code.hpp>
#include <compiler/compiler.hpp>
#include <doctest/doctest.h>
#include <eval/object.hpp>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <overloaded.hpp>
#include <parser/parser.hpp>

vm::vm(frames frames, const constants* consts, constants* globals)
    : m_constans {consts}
    , m_globals {globals}
    , m_frames {frames}
{
}

auto vm::run() -> void
{
    for (; current_frame().ip < static_cast<ssize_type>(current_frame().cl->fn->instrs.size()); current_frame().ip++) {
        const auto instr_ptr = static_cast<size_t>(current_frame().ip);
        const auto& code = current_frame().cl->fn->instrs;
        const auto op_code = static_cast<opcodes>(code[instr_ptr]);
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
                push(native_true());
                break;
            case opcodes::fals:
                push(native_false());
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
                const auto* condition = pop();
                if (!condition->is_truthy()) {
                    current_frame().ip = pos - 1;
                }

            } break;
            case opcodes::null:
                push(native_null());
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
                auto* arr = build_array(m_sp - num_elements, m_sp);
                m_sp -= num_elements;
                push(arr);
            } break;
            case opcodes::hash: {
                auto num_elements = read_uint16_big_endian(code, instr_ptr + 1UL);
                current_frame().ip += 2;

                auto* hsh = build_hash(m_sp - num_elements, m_sp);
                m_sp -= num_elements;
                push(hsh);
            } break;
            case opcodes::index: {
                const auto* index = pop();
                const auto* left = pop();
                exec_index(left, index);
            } break;
            case opcodes::call: {
                auto num_args = code[instr_ptr + 1UL];
                current_frame().ip += 1;
                exec_call(num_args);
            } break;
            case opcodes::return_value: {
                const auto* return_value = pop();
                auto frame = pop_frame();
                m_sp = static_cast<size_t>(frame.base_ptr) - 1;
                push(return_value);
            } break;
            case opcodes::ret: {
                auto& frame = pop_frame();
                m_sp = static_cast<size_t>(frame.base_ptr) - 1;
                push(native_null());
            } break;
            case opcodes::set_local: {
                auto local_index = code[instr_ptr + 1UL];
                current_frame().ip += 1;
                auto& frame = current_frame();
                m_stack[static_cast<size_t>(frame.base_ptr) + local_index] = pop();
            } break;
            case opcodes::get_local: {
                auto local_index = code[instr_ptr + 1UL];
                current_frame().ip += 1;
                auto& frame = current_frame();
                push(m_stack[static_cast<size_t>(frame.base_ptr) + local_index]);
            } break;
            case opcodes::get_builtin: {
                auto builtin_index = code[instr_ptr + 1UL];
                current_frame().ip += 1;
                const auto* const builtin = builtin_function_expression::builtins[builtin_index];
                push(make<builtin_object>(builtin));
            } break;
            case opcodes::closure: {
                auto const_idx = read_uint16_big_endian(code, instr_ptr + 1UL);
                auto num_free = code[instr_ptr + 3UL];
                current_frame().ip += 3;
                push_closure(const_idx, num_free);
            } break;
            case opcodes::get_free: {
                auto free_index = code[instr_ptr + 1UL];
                current_frame().ip += 1;
                const auto* current_closure = current_frame().cl;
                push(current_closure->free[free_index]);
            } break;
            case opcodes::current_closure: {
                push(current_frame().cl);
            } break;
            default:
                throw std::runtime_error(fmt::format("opcode {} not implemented yet", op_code));
        }
    }
}

auto vm::push(const object* obj) -> void
{
    if (m_sp >= stack_size) {
        throw std::runtime_error("stack overflow");
    }
    m_stack[m_sp] = obj;
    m_sp++;
}

auto vm::pop() -> const object*
{
    if (m_sp == 0) {
        throw std::runtime_error("stack empty");
    }
    const auto* const result = m_stack[m_sp - 1];
    m_sp--;
    return result;
}

auto vm::last_popped() const -> const object*
{
    return m_stack[m_sp];
}

auto vm::exec_binary_op(opcodes opcode) -> void
{
    const auto* right = pop();
    const auto* left = pop();
    using enum object::object_type;
    if (left->is(integer) && right->is(integer)) {
        auto left_value = left->as<integer_object>()->value;
        auto right_value = right->as<integer_object>()->value;
        switch (opcode) {
            case opcodes::sub:
                push(make<integer_object>(left_value - right_value));
                break;
            case opcodes::mul:
                push(make<integer_object>(left_value * right_value));
                break;
            case opcodes::div:
                push(make<integer_object>(left_value / right_value));
                break;
            case opcodes::add:
                push(make<integer_object>(left_value + right_value));
                break;
            default:
                throw std::runtime_error(fmt::format("unknown integer operator"));
        }
        return;
    }
    if (left->is(string) && right->is(string)) {
        const auto& left_value = left->as<string_object>()->value;
        const auto& right_value = right->as<string_object>()->value;
        switch (opcode) {
            case opcodes::add:
                push(make<string_object>(left_value + right_value));
                break;
            default:
                throw std::runtime_error(fmt::format("unknown string {} string operator", opcode));
        }
        return;
    }
    throw std::runtime_error(
        fmt::format("unsupported types for binary operation: {} {} {}", left->type(), opcode, right->type()));
}

namespace
{
template<typename ValueType>
auto exec_value_cmp(opcodes opcode, const ValueType& lhs, const ValueType& rhs) -> const object*
{
    using enum opcodes;
    switch (opcode) {
        case equal:
            return native_bool_to_object(lhs == rhs);
        case not_equal:
            return native_bool_to_object(lhs != rhs);
        case greater_than:
            return native_bool_to_object(lhs > rhs);
        default:
            throw std::runtime_error(fmt::format("unknown operator {}", opcode));
    }
}

}  // namespace

auto vm::exec_cmp(opcodes opcode) -> void
{
    const auto* right = pop();
    const auto* left = pop();
    using enum object::object_type;
    if (left->is(integer) && right->is(integer)) {
        push(exec_value_cmp(opcode, left->as<integer_object>()->value, right->as<integer_object>()->value));
        return;
    }

    if (left->is(string) && right->is(string)) {
        push(exec_value_cmp(opcode, left->as<string_object>()->value, right->as<string_object>()->value));
        return;
    }

    switch (opcode) {
        case opcodes::equal: {
            push(native_bool_to_object(left->equals_to(right)));
            return;
        }
        case opcodes::not_equal: {
            push(native_bool_to_object(!left->equals_to(right)));
            return;
        }
        default:
            throw std::runtime_error(fmt::format("unknown operator {} ({} {})", opcode, left->type(), right->type()));
    }
}

auto vm::exec_bang() -> void
{
    using enum object::object_type;
    const auto* operand = pop();
    if (operand->is(boolean)) {
        push(native_bool_to_object(!operand->as<boolean_object>()->value));
        return;
    }
    if (operand->is(null)) {
        push(native_true());
        return;
    }
    push(native_false());
}

auto vm::exec_minus() -> void
{
    const auto* operand = pop();
    if (!operand->is(object::object_type::integer)) {
        throw std::runtime_error(fmt::format("unsupported type for negation {}", operand->type()));
    }
    push(make<integer_object>(-operand->as<integer_object>()->value));
}

auto vm::build_array(size_t start, size_t end) const -> object*
{
    array_object::array arr;
    for (auto idx = start; idx < end; idx++) {
        arr.push_back(m_stack[idx]);
    }
    return make<array_object>(std::move(arr));
}

auto vm::build_hash(size_t start, size_t end) const -> object*
{
    hash_object::hash hsh;
    for (auto idx = start; idx < end; idx += 2) {
        const auto* key = m_stack[idx];
        const auto* val = m_stack[idx + 1];
        hsh[key->as<hashable_object>()->hash_key()] = val;
    }
    return make<hash_object>(std::move(hsh));
}

namespace
{
auto exec_hash(const hash_object::hash& hsh, const hashable_object::hash_key_type& key) -> const object*
{
    if (!hsh.contains(key)) {
        return native_null();
    }
    return hsh.at(key);
}
}  // namespace

auto vm::exec_index(const object* left, const object* index) -> void
{
    using enum object::object_type;
    if (left->is(array) && index->is(integer)) {
        auto idx = index->as<integer_object>()->value;
        auto max = static_cast<int64_t>(left->as<array_object>()->elements.size()) - 1;
        if (idx < 0 || idx > max) {
            push(native_null());
            return;
        }
        push(left->as<array_object>()->elements[static_cast<size_t>(idx)]);
        return;
    }
    if (left->is(string) && index->is(integer)) {
        auto idx = index->as<integer_object>()->value;
        auto max = static_cast<int64_t>(left->as<string_object>()->value.size()) - 1;
        if (idx < 0 || idx > max) {
            push(native_null());
            return;
        }
        push(make<string_object>(left->as<string_object>()->value.substr(static_cast<size_t>(idx), 1)));
        return;
    }
    if (left->is(hash) && index->is_hashable()) {
        push(exec_hash(left->as<hash_object>()->pairs, index->as<hashable_object>()->hash_key()));
        return;
    }
    push(make_error("invalid index operation: {}[{}]", left->type(), index->type()));
}

auto vm::exec_call(size_t num_args) -> void
{
    const auto* callee = m_stack[m_sp - 1 - num_args];
    using enum object::object_type;
    if (callee->is(closure)) {
        const auto* const clsr = callee->as<closure_object>();
        if (num_args != clsr->fn->num_arguments) {
            throw std::runtime_error(
                fmt::format("wrong number of arguments: want={}, got={}", clsr->fn->num_arguments, num_args));
        }
        frame frm {.cl = clsr, .ip = -1, .base_ptr = static_cast<ssize_type>(m_sp - num_args)};
        m_sp = static_cast<size_t>(frm.base_ptr) + clsr->fn->num_locals;
        push_frame(frm);
        return;
    }
    if (callee->is(builtin)) {
        const auto* const builtin = callee->as<builtin_object>()->builtin;
        array_object::array args;
        for (auto idx = m_sp - num_args; idx < m_sp; idx++) {
            args.push_back(m_stack[idx]);
        }
        const auto* result = builtin->body(std::move(args));
        push(result);
        return;
    }
    throw std::runtime_error("calling non-closure and non-builtin");
}

auto vm::current_frame() -> frame&
{
    return m_frames[m_frame_index - 1];
}

auto vm::push_frame(frame frm) -> void
{
    m_frames[m_frame_index] = frm;
    m_frame_index++;
}

auto vm::pop_frame() -> frame&
{
    m_frame_index--;
    return m_frames[m_frame_index];
}

auto vm::push_closure(uint16_t const_idx, uint8_t num_free) -> void
{
    const auto* constant = m_constans->at(const_idx);
    if (!constant->is(object::object_type::compiled_function)) {
        throw std::runtime_error("not a function");
    }
    array_object::array free;
    for (auto i = 0UL; i < num_free; i++) {
        free.push_back(m_stack[m_sp - num_free + i]);
    }
    m_sp -= num_free;
    push(make<closure_object>(constant->as<compiled_function_object>(), free));
}

namespace
{
template<typename T>
auto maker(std::initializer_list<T> list) -> std::vector<T>
{
    return std::vector<T> {list};
}

auto check_no_parse_errors(const parser& prsr) -> bool
{
    INFO("expected no errors, got:", fmt::format("{}", fmt::join(prsr.errors(), ", ")));
    CHECK(prsr.errors().empty());
    return prsr.errors().empty();
}

using parsed_program = std::pair<program*, parser>;

auto check_program(std::string_view input) -> parsed_program
{
    auto prsr = parser {lexer {input}};
    auto* prgrm = prsr.parse_program();
    INFO("while parsing: `", input, "`");
    CHECK(check_no_parse_errors(prsr));
    return {prgrm, std::move(prsr)};
}

struct error
{
    std::string message;
};

using hash = std::unordered_map<hashable_object::hash_key_type, int64_t>;
using null_type = std::monostate;
const null_type null_value {};

auto require_is(const bool expected, const object*& actual_obj, std::string_view input) -> void
{
    INFO(input,
         " expected: boolean with: ",
         expected,
         " got: ",
         actual_obj->type(),
         " with: ",
         actual_obj->inspect(),
         " instead");
    REQUIRE(actual_obj->is(object::object_type::boolean));
    const auto& actual = actual_obj->as<boolean_object>()->value;
    REQUIRE(actual == expected);
}

auto require_is(const int64_t expected, const object*& actual_obj, std::string_view input) -> void
{
    INFO(input,
         " expected: integer with: ",
         expected,
         " got: ",
         actual_obj->type(),
         " with: ",
         actual_obj->inspect(),
         " instead");
    REQUIRE(actual_obj->is(object::object_type::integer));
    const auto& actual = actual_obj->as<integer_object>()->value;
    REQUIRE(actual == expected);
}

auto require_is(const std::string& expected, const object*& actual_obj, std::string_view input) -> void
{
    INFO(input,
         " expected: string with: ",
         expected,
         " got: ",
         actual_obj->type(),
         " with: ",
         actual_obj->inspect(),
         " instead");
    REQUIRE(actual_obj->is(object::object_type::string));
    const auto& actual = actual_obj->as<string_object>()->value;
    REQUIRE(actual == expected);
}

auto require_is(const error& expected, const object*& actual_obj, std::string_view input) -> void
{
    INFO(input,
         " expected: error with: ",
         expected.message,
         " got: ",
         actual_obj->type(),
         " with: ",
         actual_obj->inspect(),
         " instead");
    REQUIRE(actual_obj->is(object::object_type::error));
    const auto& actual = actual_obj->as<error_object>()->message;
    REQUIRE(actual == expected.message);
}

auto require_array_object(const std::vector<int>& expected, const object*& actual, std::string_view input) -> void
{
    INFO(input,
         " expected: array with: ",
         expected.size(),
         " elements, got: ",
         actual->type(),
         " with: ",
         actual->inspect(),
         " instead");
    REQUIRE(actual->is(object::object_type::array));
    const auto& actual_arr = actual->as<array_object>()->elements;
    REQUIRE_EQ(actual_arr.size(), expected.size());
    for (auto idx = 0UL; const auto& elem : expected) {
        REQUIRE_EQ(elem, actual_arr[idx]->as<integer_object>()->value);
        ++idx;
    }
}

auto require_hash_object(const hash& expected, const object*& actual, std::string_view input) -> void
{
    INFO(input,
         " expected: hash with: ",
         expected.size(),
         " pairs, got: ",
         actual->type(),
         " with: ",
         actual->inspect(),
         " instead");
    REQUIRE(actual->is(object::object_type::hash));
    const auto actual_hash = actual->as<hash_object>()->pairs;
    REQUIRE_EQ(actual_hash.size(), expected.size());
}

template<typename... T>
struct vt
{
    std::string_view input;
    std::variant<T...> expected;
};

template<typename... T>
auto require_eq(const std::variant<T...>& expected, const object*& actual, std::string_view input) -> void
{
    using enum object::object_type;
    std::visit(
        overloaded {
            [&](const int64_t exp) { require_is(exp, actual, input); },
            [&](const bool exp) { require_is(exp, actual, input); },
            [&](const null_type& /*null*/) { REQUIRE(actual->is(null)); },
            [&](const std::string& exp) { require_is(exp, actual, input); },
            [&](const ::error& exp) { require_is(exp, actual, input); },
            [&](const std::vector<int>& exp) { require_array_object(exp, actual, input); },
            [&](const ::hash& exp) { require_hash_object(exp, actual, input); },
        },
        expected);
}

template<size_t N, typename... Expecteds>
auto run(std::array<vt<Expecteds...>, N> tests)
{
    for (const auto& [input, expected] : tests) {
        auto [prgrm, _] = check_program(input);
        auto cmplr = compiler::create();
        cmplr.compile(prgrm);
        auto byte_code = cmplr.byte_code();
        auto mchn = vm::create(std::move(byte_code));
        mchn.run();

        const auto* top = mchn.last_popped();
        require_eq(expected, top, input);
    }
}

// NOLINTBEGIN(*)

TEST_SUITE_BEGIN("vm");

TEST_CASE("integerArithmetics")
{
    const std::array tests {
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
    const std::array tests {
        vt<bool> {R"(true)", true},
        vt<bool> {R"(false)", false},
        vt<bool> {R"("a" < "b")", true},
        vt<bool> {R"("b" < "a")", false},
        vt<bool> {R"(1 < 2)", true},
        vt<bool> {R"(1 > 2)", false},
        vt<bool> {R"(1 < 1)", false},
        vt<bool> {R"(1 > 1)", false},
        vt<bool> {R"("a" == "a")", true},
        vt<bool> {R"(1 == 1)", true},
        vt<bool> {R"("a" != "a")", false},
        vt<bool> {R"(1 != 1)", false},
        vt<bool> {R"(1 == 2)", false},
        vt<bool> {R"(1 != 2)", true},
        vt<bool> {R"(true == true)", true},
        vt<bool> {R"(false == false)", true},
        vt<bool> {R"(true == false)", false},
        vt<bool> {R"(true != false)", true},
        vt<bool> {R"(false != true)", true},
        vt<bool> {R"((1 < 2) == true)", true},
        vt<bool> {R"((1 < 2) == false)", false},
        vt<bool> {R"((1 > 2) == true)", false},
        vt<bool> {R"((1 > 2) == false)", true},
        vt<bool> {R"(("a" > "b") == false)", true},
        vt<bool> {R"(!true)", false},
        vt<bool> {R"(!false)", true},
        vt<bool> {R"(!"a")", false},
        vt<bool> {R"(!!"a")", true},
        vt<bool> {R"(!5)", false},
        vt<bool> {R"(!!true)", true},
        vt<bool> {R"(!!false)", false},
        vt<bool> {R"(!!5)", true},
        vt<bool> {R"(!(if (false) { 5; }))", true},
        vt<bool> {R"(["a", 1] == ["b", 1])", false},
        vt<bool> {R"({"a": 1} == {"b": 1})", false},
        vt<bool> {R"(["a", 1] == ["a", 1])", true},
        vt<bool> {R"({"a": 1} == {"a": 1})", true},
    };

    run(tests);
}

TEST_CASE("conditionals")
{
    const std::array tests {
        vt<int64_t, null_type> {"if (true) { 10 }", 10},
        vt<int64_t, null_type> {"if (true) { 10 } else { 20 }", 10},
        vt<int64_t, null_type> {"if (false) { 10 } else { 20 } ", 20},
        vt<int64_t, null_type> {"if (1) { 10 }", 10},
        vt<int64_t, null_type> {"if (1 < 2) { 10 }", 10},
        vt<int64_t, null_type> {"if (1 < 2) { 10 } else { 20 }", 10},
        vt<int64_t, null_type> {"if (1 > 2) { 10 } else { 20 }", 20},
        vt<int64_t, null_type> {"if (1 > 2) { 10 }", null_value},
        vt<int64_t, null_type> {"if (false) { 10 }", null_value},
        vt<int64_t, null_type> {"if ((if (false) { 10 })) { 10 } else { 20 }", 20},
    };
    run(tests);
}

TEST_CASE("globalLetStatemets")
{
    const std::array tests {
        vt<int64_t> {"let one = 1; one", 1},
        vt<int64_t> {"let one = 1; let two = 2; one + two", 3},
        vt<int64_t> {"let one = 1; let two = one + one; one + two", 3},
    };
    run(tests);
}

TEST_CASE("stringExpression")
{
    const std::array tests {
        vt<std::string> {R"("monkey")", "monkey"},
        vt<std::string> {R"("mon" + "key")", "monkey"},
        vt<std::string> {R"("mon" + "key" + "banana")", "monkeybanana"},
        vt<std::string> {R"("mon" + "k" + "a" + "S")", "monkaS"},
    };
    run(tests);
}

TEST_CASE("arrayLiterals")
{
    const std::array tests {
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
    const std::array tests {
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
    const std::array tests {
        vt<std::string, int64_t, null_type> {"[1, 2, 3][1]", 2},
        vt<std::string, int64_t, null_type> {"[1, 2, 3][0 + 2]", 3},
        vt<std::string, int64_t, null_type> {"[[1, 1, 1]][0][0]", 1},
        vt<std::string, int64_t, null_type> {"[][0]", null_value},
        vt<std::string, int64_t, null_type> {"[1, 2, 3][99]", null_value},
        vt<std::string, int64_t, null_type> {"[1][-1]", null_value},
        vt<std::string, int64_t, null_type> {"{1: 1, 2: 2}[1]", 1},
        vt<std::string, int64_t, null_type> {"{1: 1, 2: 2}[2]", 2},
        vt<std::string, int64_t, null_type> {"{1: 1}[0]", null_value},
        vt<std::string, int64_t, null_type> {"{}[0]", null_value},
        vt<std::string, int64_t, null_type> {R"({"a": 5}["a"])", 5},
        vt<std::string, int64_t, null_type> {R"("a"[0])", "a"},
        vt<std::string, int64_t, null_type> {R"("ab"[1])", "b"},
        vt<std::string, int64_t, null_type> {R"("ab"[2])", null_value},
    };
    run(tests);
}

TEST_CASE("callFunctionsWithoutArgs")
{
    const std::array tests {
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
    const std::array tests {
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
    const std::array tests {
        vt<null_type> {
            R"(let noReturn = fn() { };
               noReturn();)",
            null_value,
        },
        vt<null_type> {
            R"(let noReturn = fn() { };
               let noReturnTwo = fn() { noReturn(); };
               noReturn();
               noReturnTwo();)",
            null_value,
        },
    };
    run(tests);
}

TEST_CASE("callFirstClassFunctions")
{
    const std::array tests {
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
    const std::array tests {
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
    const std::array tests {
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
    const std::array tests {
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
        auto [prgrm, _] = check_program(input);
        auto cmplr = compiler::create();
        cmplr.compile(prgrm);
        auto mchn = vm::create(cmplr.byte_code());
        CHECK_THROWS_WITH(mchn.run(), std::get<std::string>(expected).c_str());
    }
}

TEST_CASE("callBuiltins")
{
    const std::array tests {
        vt<int64_t, null_type, std::string, std::vector<int>> {R"(len(""))", 0},
        vt<int64_t, null_type, std::string, std::vector<int>> {R"(len("four"))", 4},
        vt<int64_t, null_type, std::string, std::vector<int>> {R"(len("hello world"))", 11},
        vt<int64_t, null_type, std::string, std::vector<int>> {R"(len([1, 2, 3]))", 3},
        vt<int64_t, null_type, std::string, std::vector<int>> {R"(len([]))", 0},
        vt<int64_t, null_type, std::string, std::vector<int>> {R"(puts("hello", "world!"))", null_value},
        vt<int64_t, null_type, std::string, std::vector<int>> {R"(first([1, 2, 3]))", 1},
        vt<int64_t, null_type, std::string, std::vector<int>> {R"(first("hello"))", "h"},
        vt<int64_t, null_type, std::string, std::vector<int>> {R"(first([]))", null_value},
        vt<int64_t, null_type, std::string, std::vector<int>> {R"(last([]))", null_value},
        vt<int64_t, null_type, std::string, std::vector<int>> {R"(last(""))", null_value},
        vt<int64_t, null_type, std::string, std::vector<int>> {R"(last("o"))", "o"},
        vt<int64_t, null_type, std::string, std::vector<int>> {R"(rest([1, 2, 3]))", maker<int>({2, 3})},
        vt<int64_t, null_type, std::string, std::vector<int>> {R"(rest("hello"))", "ello"},
        vt<int64_t, null_type, std::string, std::vector<int>> {R"(rest("lo"))", "o"},
        vt<int64_t, null_type, std::string, std::vector<int>> {R"(rest("o"))", null_value},
        vt<int64_t, null_type, std::string, std::vector<int>> {R"(rest(""))", null_value},
        vt<int64_t, null_type, std::string, std::vector<int>> {R"(rest([]))", null_value},
        vt<int64_t, null_type, std::string, std::vector<int>> {R"(push([], 1))", maker<int>({1})},
        vt<int64_t, null_type, std::string, std::vector<int>> {R"(last([1, 2, 3]))", 3},
    };
    const std::array errortests {
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
    const std::array tests {
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
    const std::array tests {
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
    const std::array tests {
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
// NOLINTEND(*)
}  // namespace
