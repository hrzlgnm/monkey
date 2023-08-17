#pragma once

#include "code.hpp"
#include "compiler.hpp"
#include "symbol_table.hpp"

constexpr auto stack_size = 2048UL;
constexpr auto globals_size = 65536UL;

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
    bytecode code;
    constants stack {stack_size};
    constants_ptr globals;
    size_t stack_pointer {0};
};

inline auto make_vm(bytecode&& code)
{
    return vm {
        .code = std::move(code),
        .globals = std::make_shared<constants>(globals_size),
    };
}

inline auto make_vm_with_state(bytecode&& code, constants_ptr globals) -> vm
{
    return vm {
        .code = std::move(code),
        .globals = std::move(globals),
    };
}
