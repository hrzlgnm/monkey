#pragma once

#include <array>
#include <cstdint>
#include <memory>

#include <code/code.hpp>
#include <compiler/compiler.hpp>
#include <compiler/symbol_table.hpp>
#include <eval/object.hpp>

constexpr auto stack_size = 2048UL;
constexpr auto globals_size = 65536UL;
constexpr auto max_frames = 1024UL;

struct frame
{
    closure_object::ptr cl {};
    ssize_t ip {};
    ssize_t base_ptr {};
};

using frames = std::array<frame, max_frames>;

struct vm
{
    inline static auto create(bytecode&& code) -> vm
    {
        return create_with_state(std::move(code), std::make_shared<constants>(globals_size));
    }

    inline static auto create_with_state(bytecode&& code, constants_ptr globals) -> vm
    {
        auto main_fn = std::make_shared<compiled_function_object>(std::move(code.instrs), 0, 0);
        auto main_clousre = std::make_shared<closure_object>(std::move(main_fn));
        const frame main_frame {.cl = std::move(main_clousre)};
        frames frms;
        frms[0] = main_frame;
        return vm {std::move(frms), std::move(code.consts), std::move(globals)};
    }

    auto run() -> void;
    auto push(const object_ptr& obj) -> void;
    [[nodiscard]] auto last_popped() const -> object_ptr;

  private:
    vm(frames&& frames, constants_ptr&& consts, constants_ptr globals);
    auto pop() -> object_ptr;
    auto exec_binary_op(opcodes opcode) -> void;
    auto exec_cmp(opcodes opcode) -> void;
    auto exec_bang() -> void;
    auto exec_minus() -> void;
    auto exec_index(const object_ptr& left, const object_ptr& index) -> void;
    auto exec_call(size_t num_args) -> void;
    [[nodiscard]] auto build_array(size_t start, size_t end) const -> object_ptr;
    [[nodiscard]] auto build_hash(size_t start, size_t end) const -> object_ptr;

    auto current_frame() -> frame&;
    auto push_frame(frame&& frm) -> void;
    auto pop_frame() -> frame&;
    auto push_closure(uint16_t const_idx, uint8_t num_free) -> void;

    constants_ptr m_constans;
    constants_ptr m_globals;
    constants m_stack {stack_size};
    size_t m_sp {0};
    frames m_frames;
    size_t m_frame_index {1};
};
