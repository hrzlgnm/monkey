#pragma once

#include <memory>

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
    auto exec_index(object&& left, object&& index) -> void;
    auto build_array(size_t start, size_t end) const -> object;
    auto build_hash(size_t start, size_t end) const -> object;
    bytecode code;
    constants stack {stack_size};
    constants_ptr globals;
    size_t stack_pointer {0};

    inline static auto create(bytecode&& code) -> vm
    {
        return vm {std::move(code), std::make_shared<constants>(globals_size)};
    }
    inline static auto create_with_state(bytecode&& code, constants_ptr globals) -> vm
    {
        return vm {std::move(code), std::move(globals)};
    }

  private:
    vm(bytecode&& code, constants_ptr globals);
};
