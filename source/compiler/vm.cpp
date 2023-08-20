#include <cstdint>
#include <stdexcept>

#include "vm.hpp"

#include <ast/builtin_function_expression.hpp>
#include <eval/object.hpp>

#include "code.hpp"
#include "compiler.hpp"

using std::move;

static const object tru {true};
static const object fals {false};

vm::vm(frames&& frames, constants_ptr&& consts, constants_ptr globals)
    : m_constans {std::move(consts)}
    , m_globals {std::move(globals)}
    , m_frames {std::move(frames)}
{
}

auto vm::stack_top() const -> object
{
    if (m_sp == 0) {
        return {};
    }
    return m_stack.at(m_sp - 1);
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
