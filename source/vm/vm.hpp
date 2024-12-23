#pragma once

#include <array>
#include <cstdint>

#include <code/code.hpp>
#include <compiler/compiler.hpp>
#include <compiler/symbol_table.hpp>
#include <gc.hpp>
#include <object/object.hpp>

constexpr size_t stack_size = 2048UL;
constexpr size_t globals_size = 65536UL;
constexpr size_t max_frames = 1024UL;

using ssize_type = std::make_signed_t<size_t>;

struct frame
{
    closure_object* cl {};
    ssize_type ip {};
    ssize_type base_ptr {};
};

using frames = std::array<frame, max_frames>;

struct vm
{
    static auto create(bytecode code) -> vm
    {
        return create_with_state(std::move(code), make<constants>(globals_size));
    }

    static auto create_with_state(bytecode code, constants* globals) -> vm
    {
        auto* main_fn = make<compiled_function_object>(std::move(code.instrs), 0ULL, 0ULL);
        auto* main_closure = make<closure_object>(main_fn);
        const frame main_frame {.cl = main_closure};
        frames frms;
        frms[0] = main_frame;
        return vm {frms, code.consts, globals};
    }

    auto run() -> void;
    auto push(const object* obj) -> void;
    [[nodiscard]] auto last_popped() const -> const object*;

  private:
    vm(frames frames, const constants* consts, constants* globals);
    auto pop() -> const object*;
    auto exec_binary_op(opcodes opcode) -> void;
    void extracted();
    auto exec_bang() -> void;
    auto exec_minus() -> void;
    auto exec_index(const object* left, const object* index) -> void;
    auto exec_call(size_t num_args) -> void;
    [[nodiscard]] auto build_array(size_t start, size_t end) const -> object*;
    [[nodiscard]] auto build_hash(size_t start, size_t end) const -> object*;

    auto current_frame() -> frame&;
    auto push_frame(frame frm) -> void;
    auto pop_frame() -> frame&;
    auto push_closure(uint16_t const_idx, uint8_t num_free) -> void;

    const constants* m_constants {};
    constants* m_globals {};
    constants m_stack {stack_size};
    size_t m_sp {0};
    frames m_frames;
    size_t m_frame_index {1};
};
