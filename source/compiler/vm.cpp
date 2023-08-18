#include <cstdint>
#include <stdexcept>

#include "vm.hpp"

#include <eval/object.hpp>

#include "code.hpp"

static const object tru {true};
static const object fals {false};

vm::vm(bytecode&& code, constants_ptr globals)
    : code {std::move(code)}
    , globals {std::move(globals)}
{
}

auto vm::stack_top() const -> object
{
    if (stack_pointer == 0) {
        return {};
    }
    return stack.at(stack_pointer - 1);
}

auto vm::run() -> void
{
    for (auto ip = 0U; ip < code.instrs.size(); ip++) {
        const auto op_code = static_cast<opcodes>(code.instrs.at(ip));
        switch (op_code) {
            case opcodes::constant: {
                auto const_idx = read_uint16_big_endian(code.instrs, ip + 1UL);
                ip += 2;
                push(code.consts->at(const_idx));
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
                auto pos = read_uint16_big_endian(code.instrs, ip + 1UL);
                ip = pos - 1;
            } break;
            case opcodes::jump_not_truthy: {
                auto pos = read_uint16_big_endian(code.instrs, ip + 1UL);
                ip += 2;
                auto condition = pop();
                if (!condition.is_truthy()) {
                    ip = pos - 1;
                }

            } break;
            case opcodes::null:
                push(nil);
                break;
            case opcodes::set_global: {
                auto global_index = read_uint16_big_endian(code.instrs, ip + 1UL);
                ip += 2;
                globals->at(global_index) = pop();
            } break;
            case opcodes::get_global: {
                auto global_index = read_uint16_big_endian(code.instrs, ip + 1UL);
                ip += 2;
                push(globals->at(global_index));
            } break;
            case opcodes::array: {
                auto num_elements = read_uint16_big_endian(code.instrs, ip + 1UL);
                ip += 2;
                auto arr = build_array(stack_pointer - num_elements, stack_pointer);
                stack_pointer -= num_elements;
                push(arr);
            } break;
            case opcodes::hash: {
                auto num_elements = read_uint16_big_endian(code.instrs, ip + 1UL);
                ip += 2;

                auto hsh = build_hash(stack_pointer - num_elements, stack_pointer);
                stack_pointer -= num_elements;
                push(hsh);
            } break;
            case opcodes::index: {
                auto index = pop();
                auto left = pop();
                exec_index(std::move(left), std::move(index));
            } break;
            default:
                throw std::runtime_error(fmt::format("opcode {} not implemented yet", op_code));
        }
    }
}

auto vm::push(const object& obj) -> void
{
    if (stack_pointer >= stack_size) {
        throw std::runtime_error("stack overflow");
    }
    stack[stack_pointer] = obj;
    stack_pointer++;
}

auto vm::pop() -> object
{
    if (stack_pointer == 0) {
        throw std::runtime_error("stack empty");
    }
    auto result = stack[stack_pointer - 1];
    stack_pointer--;
    return result;
}

auto vm::last_popped() const -> object
{
    return stack[stack_pointer];
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
        arr.push_back(stack.at(idx));
    }
    return {arr};
}

auto vm::build_hash(size_t start, size_t end) const -> object
{
    hash hsh;
    for (auto idx = start; idx < end; idx += 2) {
        auto key = stack[idx];
        auto val = stack[idx + 1];
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
