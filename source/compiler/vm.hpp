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

    constants consts {};
    instructions code {};
    constants stack {stack_size};
    size_t stack_pointer {0};
};
