#pragma once

#include <array>
#include <cstdint>

#include <code/code.hpp>
#include <compiler/compiler.hpp>
#include <compiler/symbol_table.hpp>
#include <gc.hpp>
#include <object/object.hpp>

constexpr size_t stack_size = 2 * 2048UL;
constexpr size_t globals_size = 65536UL;
constexpr size_t max_frames = 1024UL;

struct frame final
{
    closure_object* cl {};
    int ip {};
    int base_ptr {};
};

using frames = std::array<frame, max_frames>;

struct vm final
{
    static auto create(bytecode code) -> vm;
    static auto create_with_state(bytecode code, constants* globals) -> vm;
    auto run() -> void;
    [[nodiscard]] auto last_popped() const -> const object*;

  private:
    vm(frames frames, const constants* consts, constants* globals);
    auto push(const object* obj) -> void;
    auto pop() -> const object*;
    auto exec_binary_op(opcodes opcode) -> void;
    auto exec_bang() -> void;
    auto exec_minus() -> void;
    auto exec_index(const object* left, const object* index) -> void;
    auto exec_call(int num_args) -> void;
    void exec_set_outer(int ip, const instructions& instr);
    void exec_get_outer(int ip, const instructions& instr);
    [[nodiscard]] auto build_array(int start, int end) const -> const object*;
    [[nodiscard]] auto build_hash(int start, int end) const -> const object*;
    auto current_frame() -> frame&;
    auto push_frame(frame frm) -> void;
    auto pop_frame() -> frame&;
    auto push_closure(uint16_t const_idx, uint8_t num_free) -> void;

    const constants* m_constants {};
    constants* m_globals {};
    constants m_stack {stack_size};
    int m_sp {0};
    frames m_frames;
    int m_frame_index {1};
};
