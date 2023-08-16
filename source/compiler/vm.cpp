#include <cstdint>
#include <stdexcept>

#include "vm.hpp"

#include "code.hpp"
#include "object.hpp"
#include "token_type.hpp"
#include "util.hpp"

static const object tru {true};
static const object fals {false};

auto vm::stack_top() const -> object
{
    if (stack_pointer == 0) {
        return {};
    }
    return stack.at(stack_pointer - 1);
}

auto vm::run() -> void
{
    for (auto ip = 0U; ip < code.size(); ip++) {
        const auto op_code = static_cast<opcodes>(code.at(ip));
        switch (op_code) {
            case opcodes::constant: {
                auto const_idx = read_uint16_big_endian(code, ip + 1UL);
                ip += 2;
                push(consts.at(const_idx));
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
            default:
                throw std::runtime_error(fmt::format("Invalid op code {}", static_cast<uint8_t>(op_code)));
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
    if (left.is<integer_value>() && right.is<integer_value>()) {
        auto left_value = left.as<integer_value>();
        auto right_value = right.as<integer_value>();
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
    throw std::runtime_error(
        fmt::format("unsupported types for binary operation: {} {}", left.type_name(), right.type_name()));
}

auto exec_int_cmp(opcodes opcode, integer_value lhs, integer_value rhs) -> bool
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
            throw std::runtime_error("unknown operator");
    }
}

auto vm::exec_cmp(opcodes opcode) -> void
{
    auto right = pop();
    auto left = pop();
    if (left.is<integer_value>() && right.is<integer_value>()) {
        push({exec_int_cmp(opcode, left.as<integer_value>(), right.as<integer_value>())});
        return;
    }

    using enum opcodes;
    switch (opcode) {
        case equal:
            return push({left == right});
        case not_equal:
            return push({left != right});
        default:
            throw std::runtime_error(fmt::format(
                "unknown operator {} ({} {})", static_cast<uint8_t>(opcode), left.type_name(), right.type_name()));
    }
}
