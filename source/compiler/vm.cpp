#include <cstdint>
#include <stdexcept>

#include "vm.hpp"

#include "code.hpp"
#include "object.hpp"
#include "util.hpp"

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
            case opcodes::add: {
                auto right = pop();
                auto left = pop();
                auto left_value = left.as<integer_value>();
                auto right_value = right.as<integer_value>();
                auto result = left_value + right_value;
                push({result});
            } break;
            case opcodes::pop:
                pop();
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
