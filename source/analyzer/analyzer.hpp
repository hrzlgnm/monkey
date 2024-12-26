#pragma once

#include <ast/program.hpp>
#include <compiler/symbol_table.hpp>

void analyze_program(const program* program, symbol_table* existing_symbols) noexcept(false);
[[noreturn]] void fail(const std::string& error_message);
