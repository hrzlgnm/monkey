#include <stdexcept>

#include "analyzer.hpp"

#include <ast/builtin_function.hpp>
#include <ast/program.hpp>
#include <compiler/symbol_table.hpp>
#include <eval/environment.hpp>

void analyze_program(const program* program,
                     symbol_table* existing_symbols,
                     const environment* existing_env) noexcept(false)
{
    symbol_table* symbols = nullptr;
    if (existing_symbols != nullptr) {
        symbols = symbol_table::create_enclosed(existing_symbols);
    } else {
        symbols = symbol_table::create();
        for (auto i = 0; const auto* builtin : builtin_function::builtins()) {
            symbols->define_builtin(i++, builtin->name);
        }
    }
    if (existing_env != nullptr) {
        for (const auto& [key, _] : existing_env->store) {
            symbols->define(key);
        }
    }
    program->check(symbols);
}

void fail(const std::string& error_message)
{
    throw std::runtime_error(error_message);
}
