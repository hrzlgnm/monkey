#pragma once

#include <array>
#include <cstdint>
#include <memory>

#include "code.hpp"
#include "compiler.hpp"
#include "symbol_table.hpp"

constexpr auto stack_size = 2048UL;
constexpr auto globals_size = 65536UL;
constexpr auto max_frames = 1024UL;

struct frame
{
    closure cl {};
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
        auto main_fn = compiled_function {.instrs = std::move(code.instrs)};
        closure main_clousre {.fn = std::move(main_fn), .free {}};
        frame main_frame {.cl = std::move(main_clousre)};
        frames frms;
        frms[0] = main_frame;
        return vm {std::move(frms), std::move(code.consts), std::move(globals)};
    }

    auto run() -> void;
    auto push(const object& obj) -> void;
    [[nodiscard]] auto last_popped() const -> object;

  private:
    vm(frames&& frames, constants_ptr&& consts, constants_ptr globals);
    auto pop() -> object;
    auto exec_binary_op(opcodes opcode) -> void;
    auto exec_cmp(opcodes opcode) -> void;
    auto exec_bang() -> void;
    auto exec_minus() -> void;
    auto exec_index(object&& left, object&& index) -> void;
    auto exec_call(size_t num_args) -> void;
    [[nodiscard]] auto build_array(size_t start, size_t end) const -> object;
    [[nodiscard]] auto build_hash(size_t start, size_t end) const -> object;

    auto current_frame() -> frame&;
    auto push_frame(frame&& frm) -> void;
    auto pop_frame() -> frame;
    auto push_closure(uint16_t const_idx, uint8_t num_free) -> void;

    constants_ptr m_constans;
    constants_ptr m_globals;
    constants m_stack {stack_size};
    size_t m_sp {0};
    frames m_frames;
    size_t m_frame_index {1};
};
