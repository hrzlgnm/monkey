#pragma once

#include <ast/program.hpp>
#include <compiler/symbol_table.hpp>
#include <eval/environment.hpp>

void analyze_program(const program* program,
                     symbol_table* existing_symbols,
                     const environment* existing_env) noexcept(false);
[[noreturn]] void fail(const std::string& error_message);
