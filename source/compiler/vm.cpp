#include "vm.hpp"

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
            case opcodes::constant:
                auto const_idx = code.at(ip + 1) * 256 + code.at(ip + 2);
                ip += 2;
                push(consts.at(const_idx));
                break;
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
