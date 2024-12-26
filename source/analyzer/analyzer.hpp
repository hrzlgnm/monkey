#pragma once

#include "ast/program.hpp"
#include "compiler/symbol_table.hpp"

struct analyzer
{
    analyzer() = default;

    void analyze_program(const program* program, symbol_table* existing_symbols) noexcept(false);
    [[noreturn]] void fail(const std::string& error_message) const;
};
