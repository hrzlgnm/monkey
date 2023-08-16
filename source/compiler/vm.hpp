#pragma once

#include "code.hpp"
#include "compiler.hpp"
constexpr auto stack_size = 2048;

struct vm
{
    auto stack_top() const -> object;
    auto run() -> void;
    auto push(const object& obj) -> void;
    auto pop() -> object;
    auto last_popped() const -> object;
    auto exec_binary_op(opcodes opcode) -> void;
    auto exec_cmp(opcodes opcode) -> void;
    auto exec_bang() -> void;
    auto exec_minus() -> void;
    constants consts {};
    instructions code {};
    constants stack {stack_size};
    size_t stack_pointer {0};
};
