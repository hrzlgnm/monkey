#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "vm.hpp"

#include <ast/program.hpp>
#include <builtin/builtin.hpp>
#include <code/code.hpp>
#include <compiler/compiler.hpp>
#include <compiler/symbol_table.hpp>
#include <doctest/doctest.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <gc.hpp>
#include <lexer/lexer.hpp>
#include <object/object.hpp>
#include <overloaded.hpp>
#include <parser/parser.hpp>

auto vm::create(bytecode code) -> vm
{
    return create_with_state(std::move(code), make<constants>(globals_size));
}

auto vm::create_with_state(bytecode code, constants* globals) -> vm
{
    auto* main_fn = make<compiled_function_object>(std::move(code.instrs), 0, 0);
    auto* main_closure = make<closure_object>(main_fn);
    const frame main_frame {.cl = main_closure};
    frames frms;
    frms[0] = main_frame;
    return vm {frms, code.consts, globals};
}

vm::vm(frames frames, const constants* consts, constants* globals)
    : m_constants {consts}
    , m_globals {globals}
    , m_frames {frames}
{
}

auto vm::run() -> void
{
    for (; current_frame().ip < current_frame().cl->fn->instrs.size(); current_frame().ip++) {
        const auto ip = current_frame().ip;
        const auto& instr = current_frame().cl->fn->instrs;
        const auto op = static_cast<opcodes>(instr[ip]);
        switch (op) {
            case opcodes::constant: {
                current_frame().ip += 2;
                const auto const_idx = read_uint16_big_endian(instr, ip + 1UL);
                const auto* constant = (*m_constants)[const_idx];
                if (constant == nullptr) {
                    throw std::runtime_error(fmt::format("constant at index {} does not exist", const_idx));
                }
                push((*m_constants)[const_idx]);
            } break;
            case opcodes::add:
            case opcodes::sub:
            case opcodes::mul:
            case opcodes::div:
            case opcodes::mod:
            case opcodes::floor_div:
            case opcodes::bit_and:
            case opcodes::bit_or:
            case opcodes::bit_xor:
            case opcodes::bit_lsh:
            case opcodes::bit_rsh:
            case opcodes::logical_and:
            case opcodes::logical_or:
            case opcodes::equal:
            case opcodes::not_equal:
            case opcodes::greater_than:
                exec_binary_op(op);
                break;
            case opcodes::pop:
                pop();
                break;
            case opcodes::tru:
                push(tru());
                break;
            case opcodes::fals:
                push(fals());
                break;
            case opcodes::bang:
                exec_bang();
                break;
            case opcodes::minus:
                exec_minus();
                break;
            case opcodes::jump: {
                current_frame().ip = read_uint16_big_endian(instr, ip + 1UL) - 1;
            } break;
            case opcodes::jump_not_truthy: {
                current_frame().ip += 2;
                const auto* condition = pop();
                if (!condition->is_truthy()) {
                    current_frame().ip = read_uint16_big_endian(instr, ip + 1UL) - 1;
                }
            } break;
            case opcodes::null:
                push(null());
                break;
            case opcodes::set_global: {
                current_frame().ip += 2;
                const auto global_index = read_uint16_big_endian(instr, ip + 1UL);
                (*m_globals)[global_index] = pop();
            } break;
            case opcodes::get_global: {
                auto global_index = read_uint16_big_endian(instr, ip + 1UL);
                current_frame().ip += 2;
                const auto* global = (*m_globals)[global_index];
                if (global == nullptr) {
                    throw std::runtime_error(fmt::format("global at index {} does not exits", global_index));
                }
                push(global);
            } break;
            case opcodes::array: {
                current_frame().ip += 2;
                const auto num_elements = read_uint16_big_endian(instr, ip + 1UL);
                const auto* arr = build_array(m_sp - num_elements, m_sp);
                m_sp -= num_elements;
                push(arr);
            } break;
            case opcodes::hash: {
                current_frame().ip += 2;
                const auto num_elements = read_uint16_big_endian(instr, ip + 1UL);
                const auto* hsh = build_hash(m_sp - num_elements, m_sp);
                m_sp -= num_elements;
                push(hsh);
            } break;
            case opcodes::index: {
                const auto* index = pop();
                const auto* left = pop();
                exec_index(left, index);
            } break;
            case opcodes::call: {
                current_frame().ip += 1;
                const auto num_args = instr[ip + 1UL];
                exec_call(num_args);
            } break;
            case opcodes::brake: {
                current_frame().ip += 1;
                auto& frame = pop_frame();
                m_sp = frame.base_ptr - 1;
                push(fals());
            } break;
            case opcodes::cont: {
                current_frame().ip += 1;
                auto& frame = pop_frame();
                m_sp = frame.base_ptr - 1;
                push(tru());
            } break;
            case opcodes::return_value: {
                const auto* return_value = pop();
                auto& frame = pop_frame();
                while (frame.cl->fn->inside_loop) {
                    frame = pop_frame();
                }
                m_sp = frame.base_ptr - 1;
                push(return_value);
            } break;
            case opcodes::ret: {
                auto& frame = pop_frame();
                m_sp = frame.base_ptr - 1;
                push(null());
            } break;
            case opcodes::set_local: {
                current_frame().ip += 1;
                const auto local_index = instr[ip + 1UL];
                auto& frame = current_frame();
                m_stack[frame.base_ptr + local_index] = pop();
            } break;
            case opcodes::get_local: {
                current_frame().ip += 1;
                const auto local_index = instr[ip + 1UL];
                auto& frame = current_frame();
                push(m_stack[frame.base_ptr + local_index]);
            } break;
            case opcodes::set_outer:
                current_frame().ip += 3;
                exec_set_outer(ip, instr);
                break;
            case opcodes::get_outer:
                current_frame().ip += 3;
                exec_get_outer(ip, instr);
                break;
            case opcodes::get_builtin: {
                current_frame().ip += 1;
                const auto builtin_index = instr[ip + 1UL];
                const auto* const builtin = builtin::builtins()[builtin_index];
                push(make<builtin_object>(builtin));
            } break;
            case opcodes::set_free: {
                current_frame().ip += 1;
                const auto free_index = instr[ip + 1UL];
                current_frame().cl->free[free_index] = pop();
            } break;
            case opcodes::get_free: {
                current_frame().ip += 1;
                const auto free_index = instr[ip + 1UL];
                const auto* current_closure = current_frame().cl;
                push(current_closure->free[free_index]);
            } break;
            case opcodes::closure: {
                current_frame().ip += 3;
                const auto const_idx = read_uint16_big_endian(instr, ip + 1UL);
                const auto num_free = instr[ip + 3UL];
                push_closure(const_idx, num_free);
            } break;
            case opcodes::current_closure: {
                push(current_frame().cl);
            } break;
        }
    }
}

auto vm::push(const object* obj) -> void
{
    assert(obj != nullptr);
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

namespace
{

auto apply_binary_operator(opcodes opcode, const object* left, const object* right) -> const object*
{
    using enum opcodes;
    switch (opcode) {
        case add:
            return *left + *right;
        case mul:
            return *left * *right;
        case sub:
            return *left - *right;
        case div:
            return *left / *right;
        case mod:
            return *left % *right;
        case bit_and:
            return *left & *right;
        case bit_or:
            return *left | *right;
        case bit_xor:
            return *left ^ *right;
        case bit_lsh:
            return *left << *right;
        case bit_rsh:
            return *left >> *right;
        case logical_and:
            return *left && *right;
        case logical_or:
            return *left || *right;
        case equal:
            return *left == *right;
        case not_equal:
            return *left != *right;
        case greater_than:
            return *left > *right;
        case floor_div:
            return object_floor_div(left, right);
        default:
            return nullptr;
    }
}
}  // namespace

auto vm::exec_binary_op(opcodes opcode) -> void
{
    const auto* right = pop();
    const auto* left = pop();
    if (const auto* result = apply_binary_operator(opcode, left, right); result != nullptr) {
        push(result);
        return;
    }
    throw std::runtime_error(
        fmt::format("unsupported types for binary operation: {} {} {}", left->type(), opcode, right->type()));
}

auto vm::exec_bang() -> void
{
    using enum object::object_type;
    const auto* operand = pop();
    push(native_bool_to_object(!operand->is_truthy()));
}

auto vm::exec_minus() -> void
{
    const auto* operand = pop();
    if (operand->is(object::object_type::integer)) {
        push(make<integer_object>(-operand->as<integer_object>()->value));
        return;
    }
    if (operand->is(object::object_type::decimal)) {
        push(make<decimal_object>(-operand->as<decimal_object>()->value));
        return;
    }

    throw std::runtime_error(fmt::format("unsupported type for negation {}", operand->type()));
}

void vm::exec_set_outer(const int ip, const instructions& instr)
{
    const auto level = instr[ip + 1UL];
    const auto scope = static_cast<symbol_scope>(instr[ip + 2UL]);
    const auto index = instr[ip + 3UL];
    const auto& frame = m_frames[m_frame_index - (level + 1)];
    if (scope == symbol_scope::local) {
        m_stack[frame.base_ptr + index] = pop();
    } else if (scope == symbol_scope::free) {
        frame.cl->free[index] = pop();
    }
}

void vm::exec_get_outer(const int ip, const instructions& instr)
{
    const auto level = instr[ip + 1UL];
    const auto scope = static_cast<symbol_scope>(instr[ip + 2UL]);
    const auto index = instr[ip + 3UL];
    const auto& frame = m_frames[m_frame_index - (level + 1)];
    if (scope == symbol_scope::local) {
        push(m_stack[frame.base_ptr + index]);
    } else if (scope == symbol_scope::free) {
        push(frame.cl->free[index]);
    } else if (scope == symbol_scope::function) {
        push(frame.cl);
    }
}

auto vm::build_array(int start, int end) const -> const object*
{
    array_object::value_type arr;
    for (auto idx = start; idx < end; idx++) {
        arr.push_back(m_stack[idx]);
    }
    return make<array_object>(std::move(arr));
}

auto vm::build_hash(int start, int end) const -> const object*
{
    hash_object::value_type hsh;
    for (auto idx = start; idx < end; idx += 2) {
        const auto* key = m_stack[idx];
        const auto* val = m_stack[idx + 1];
        hsh[key->as<hashable_object>()->hash_key()] = val;
    }
    return make<hash_object>(std::move(hsh));
}

namespace
{
auto exec_hash(const hash_object::value_type& hsh, const hashable_object::hash_key_type& key) -> const object*
{
    if (const auto itr = hsh.find(key); itr != hsh.end()) {
        return itr->second;
    }
    return null();
}
}  // namespace

auto vm::exec_index(const object* left, const object* index) -> void
{
    using enum object::object_type;
    if (left->is(array) && index->is(integer)) {
        auto idx = index->as<integer_object>()->value;
        auto max = static_cast<int64_t>(left->as<array_object>()->value.size()) - 1;
        if (idx < 0 || idx > max) {
            push(null());
            return;
        }
        push(left->as<array_object>()->value[static_cast<std::size_t>(idx)]);
        return;
    }
    if (left->is(string) && index->is(integer)) {
        auto idx = index->as<integer_object>()->value;
        auto max = static_cast<int64_t>(left->as<string_object>()->value.size()) - 1;
        if (idx < 0 || idx > max) {
            push(null());
            return;
        }
        push(make<string_object>(left->as<string_object>()->value.substr(static_cast<std::size_t>(idx), 1)));
        return;
    }
    if (left->is(hash) && index->is_hashable()) {
        push(exec_hash(left->as<hash_object>()->value, index->as<hashable_object>()->hash_key()));
        return;
    }
    push(make_error("invalid index operation: {}[{}]", left->type(), index->type()));
}

auto vm::exec_call(int num_args) -> void
{
    const auto* callee = m_stack[m_sp - 1 - num_args];
    using enum object::object_type;
    if (callee->is(closure)) {
        const auto* clsr = callee->as<closure_object>();
        if (num_args != clsr->fn->num_arguments) {
            throw std::runtime_error(
                fmt::format("wrong number of arguments: want={}, got={}", clsr->fn->num_arguments, num_args));
        }
        const frame frm {.cl = clsr->as_mutable(), .ip = -1, .base_ptr = m_sp - num_args};
        m_sp = frm.base_ptr + clsr->fn->num_locals;
        push_frame(frm);
        return;
    }
    if (callee->is(builtin)) {
        const auto* const builtin = callee->as<builtin_object>()->builtin;
        array_object::value_type args;
        for (auto idx = m_sp - num_args; idx < m_sp; idx++) {
            args.push_back(m_stack[idx]);
        }
        m_sp = m_sp - num_args - 1;
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
    const auto* constant = (*m_constants)[const_idx];
    if (!constant->is(object::object_type::compiled_function)) {
        throw std::runtime_error(
            fmt::format("expected a compiled_function, got an object of type {}", constant->type()));
    }
    array_object::value_type free;
    for (auto i = 0UL; i < num_free; i++) {
        free.push_back(m_stack[m_sp - num_free + i]);
    }
    m_sp -= num_free;
    push(make<closure_object>(constant->as<compiled_function_object>(), free));
}

namespace
{
namespace dt = doctest;

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

auto require_is(const double expected, const object*& actual_obj, std::string_view input) -> void
{
    INFO(input,
         " expected: decimal with: ",
         expected,
         " got: ",
         actual_obj->type(),
         " with: ",
         actual_obj->inspect(),
         " instead");
    REQUIRE(actual_obj->is(object::object_type::decimal));
    const auto& actual = actual_obj->as<decimal_object>()->value;
    REQUIRE(actual == dt::Approx(expected));
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
    const auto& actual = actual_obj->as<error_object>()->value;
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
    const auto& actual_arr = actual->as<array_object>()->value;
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
    const auto actual_hash = actual->as<hash_object>()->value;
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
            [&](const double exp) { require_is(exp, actual, input); },
            [&](const bool exp) { require_is(exp, actual, input); },
            [&](const null_type& /*null*/) { REQUIRE(actual->is_null()); },
            [&](const std::string& exp) { require_is(exp, actual, input); },
            [&](const ::error& exp) { require_is(exp, actual, input); },
            [&](const std::vector<int>& exp) { require_array_object(exp, actual, input); },
            [&](const ::hash& exp) { require_hash_object(exp, actual, input); },
        },
        expected);
}

template<std::size_t N, typename... Expecteds>
auto run(const std::array<vt<Expecteds...>, N>& tests)
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

        vt<int64_t> {"5 % 2", 1},
        vt<int64_t> {"5 % 5", 0},
        vt<int64_t> {"5 & 5", 5},
        vt<int64_t> {"5 & true", 1},
        vt<int64_t> {"true & 3", 1},
        vt<int64_t> {"5 | 5", 5},
        vt<int64_t> {"5 | true", 5},
        vt<int64_t> {"true | 3", 3},
        vt<int64_t> {"5 ^ 5", 0},
        vt<int64_t> {"5 ^ true", 4},
        vt<int64_t> {"true ^ 3", 2},
        vt<int64_t> {"true << 3", 8},
        vt<int64_t> {"true >> 3", 0},
        vt<int64_t> {"50 + 2 * 2 + 10 - 5", 59},
        vt<int64_t> {"5 + 5 + 5 + 5 - 10", 10},
        vt<int64_t> {"2 * 2 * 2 * 2 * 2", 32},
        vt<int64_t> {"5 * 2 + 10", 20},
        vt<int64_t> {"5 + 2 * 10", 25},
        vt<int64_t> {"5 * (2 + 10)", 60},
        vt<int64_t> {"-5", -5},
        vt<int64_t> {"-10", -10},
        vt<int64_t> {"-50 + 100 + -50", 0},
        vt<int64_t> {"-50 + 100 + -50", 0},
        vt<int64_t> {"(5 + 10 * 2 + 15 + 3) * 2 + -10", 76},
    };
    run(tests);
}

TEST_CASE("decimalArithmetics")
{
    const std::array tests {
        vt<double> {"4 / 2", 2.0},
        vt<double> {"5 // 2", 2.0},
        vt<double> {"1.1", 1.1},
        vt<double> {"-2.2", -2.2},
        vt<double> {"(5.0 + 10 * 2 + 15 / 3) * 2 + -10", 50.0},
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
        vt<bool> {R"(true & true)", true},
        vt<bool> {R"(false & true)", false},
        vt<bool> {R"(true | true)", true},
        vt<bool> {R"(false | true)", true},
        vt<bool> {R"(false | false)", false},
        vt<bool> {R"(true ^ true)", false},
        vt<bool> {R"(false ^ true)", true},
        vt<bool> {R"(false ^ false)", false},
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
        vt<bool> {R"(!!"")", false},
        vt<bool> {R"(!"a")", false},
        vt<bool> {R"(!!"a")", true},
        vt<bool> {R"(!0)", true},
        vt<bool> {R"(![])", true},
        vt<bool> {R"(!{})", true},
        vt<bool> {R"(!5)", false},
        vt<bool> {R"(!!true)", true},
        vt<bool> {R"(!!false)", false},
        vt<bool> {R"(!!5)", true},
        vt<bool> {R"(!(if (false) { 5; }))", true},
        vt<bool> {R"(["a", 1] == ["b", 1])", false},
        vt<bool> {R"({"a": 1} == {"b": 1})", false},
        vt<bool> {R"(["a", 1] == ["a", 1])", true},
        vt<bool> {R"({"a": 1} == {"a": 1})", true},
        vt<bool> {R"(1 && "a")", true},
        vt<bool> {R"(1 || "a")", true},
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

TEST_CASE("globalLetStatements")
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
        vt<std::string> {R"("cappuchin")", "cappuchin"},
        vt<std::string> {R"("cappu" + "chin)", "cappuchin"},
        vt<std::string> {R"("cappu" + "chin" + "banana")", "cappuchinbanana"},
        vt<std::string> {R"("cappu" + "c" + "h" + "i")", "cappuchi"},
    };
    run(tests);
}

TEST_CASE("stringIntMultiplication")
{
    const std::array tests {
        vt<std::string> {R"("cappuchin" * 2)", "cappuchincappuchin"},
        vt<std::string> {R"(2 * "cappu" + "chin" * 3)", "cappucappuchinchinchin"},
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
        vt<std::vector<int>> {
            "2 * [1, 3, 5]",
            {std::vector<int> {1, 3, 5, 1, 3, 5}},
        },
        vt<std::vector<int>> {
            "[1, 5] * 3",
            {std::vector<int> {1, 5, 1, 5, 1, 5}},
        },
        vt<std::vector<int>> {
            "[1, 5] + [3]",
            {std::vector<int> {1, 5, 3}},
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
        vt<int64_t, null_type, std::string, std::vector<int>> {R"(type([]))", "array"},
        vt<int64_t, null_type, std::string, std::vector<int>> {R"(push([], first([1])))", maker<int>({1})},
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

TEST_CASE("recursiveFibonacci")
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
