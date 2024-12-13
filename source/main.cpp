#include <cstdint>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <iterator>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <ast/builtin_function_expression.hpp>
#include <chungus.hpp>
#include <compiler/compiler.hpp>
#include <eval/environment.hpp>
#include <lexer/lexer.hpp>
#include <parser/parser.hpp>
#include <vm/vm.hpp>

#include "eval/object.hpp"

namespace
{
constexpr auto prompt = ">> ";

constexpr auto monkey_face = R"r(
             __,__
     .--. .-"     "-. .--.
    / .. \/ .-. .-. \/ .. \
   | |  '| /   Y   \ |'  | |
   | \   \ \ 0 | 0 / /  /  |
    \ '-,\.-"""""""-./,-' /
     ''-' /_  ^ ^  _\ '-''
         | \._   _./ |
         \  \ '~' /  /
          '._'-=-'_.'
            '-----'
)r";

auto monkey_business()
{
    std::cerr << monkey_face;
    std::cerr << "Woops! We ran into some monkey business here!\n";
}

auto print_parse_errors(const std::vector<std::string>& errors)
{
    monkey_business();
    std::cerr << "  parser errors: \n";
    for (const auto& error : errors) {
        std::cerr << "\t" << error << "\n";
    }
}

enum class engine : std::uint8_t
{
    vm,
    eval,
};

struct command_line_args
{
    bool help {};
    bool debug_env {};
    engine mode {};
    std::string_view file;
};

[[noreturn]] auto show_usage(std::string_view program, std::string_view error_msg = {})
{
    auto exit_code = EXIT_SUCCESS;
    if (!error_msg.empty()) {
        fmt::print("Error: {}\n", error_msg);
        exit_code = EXIT_FAILURE;
    }
    fmt::print("Usage: {} [-d] [-i] [-h] [<file>]\n\n", program);
    exit(exit_code);
}

auto parse_command_line(std::string_view program, int argc, char** argv) -> command_line_args
{
    command_line_args opts {};
    for (std::string_view arg : std::span(argv, static_cast<size_t>(argc))) {
        if (arg.at(0) == '-' && arg.size() == 1) {
            show_usage(program, fmt::format("invalid option {}", arg));
        }
        if (arg.at(0) == '-' && arg.size() > 1) {
            switch (arg.at(1)) {
                case 'i':
                    opts.mode = engine::eval;
                    break;
                case 'h':
                    opts.help = true;
                    break;
                case 'd':
                    opts.debug_env = true;
                    break;
                default: {
                    show_usage(program, fmt::format("invalid option {}", arg));
                }
            }
        } else {
            if (opts.file.empty()) {
                opts.file = arg;
            } else {
                fmt::print("ignoring file argument {}, already have one set.\n", arg);
            }
        }
    }
    return opts;
}

auto run_file(const command_line_args& opts) -> int
{
    std::ifstream ifs(std::string {opts.file});
    if (!ifs) {
        std::cerr << "ERROR: could not open file: " << opts.file << "\n";
        return 1;
    }
    std::string contents {(std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>())};
    auto lxr = lexer {contents};
    auto prsr = parser {lxr};
    auto* prgrm = prsr.parse_program();
    if (!prsr.errors().empty()) {
        print_parse_errors(prsr.errors());
        return 1;
    }
    if (opts.mode == engine::vm) {
        auto cmplr = compiler::create();
        cmplr.compile(prgrm);
        auto machine = vm::create(cmplr.byte_code());
        machine.run();
        const auto* result = machine.last_popped();
        if (!result->is(object::object_type::null)) {
            std::cout << result->inspect() << "\n";
        }
    } else {
        auto* global_env = make<environment>();
        for (const auto& builtin : builtin_function_expression::builtins) {
            global_env->set(builtin->name, make<function_object>(builtin, nullptr));
        }
        const auto* result = prgrm->eval(global_env);
        if (!result->is(object::object_type::null)) {
            std::cout << result->inspect() << "\n";
        }
    }
    return 0;
}

auto run_repl(const command_line_args& opts) -> int
{
    auto* global_env = make<environment>();
    auto* symbols = symbol_table::create();
    constants consts;
    constants globals(globals_size);
    for (auto idx = 0UL; const auto& builtin : builtin_function_expression::builtins) {
        global_env->set(builtin->name, make<function_object>(builtin, nullptr));
        symbols->define_builtin(idx, builtin->name);
        idx++;
    }
    auto input = std::string {};
    std::cout << prompt;
    while (getline(std::cin, input)) {
        auto lxr = lexer {input};
        auto prsr = parser {lxr};
        auto* prgrm = prsr.parse_program();
        if (!prsr.errors().empty()) {
            print_parse_errors(prsr.errors());
            continue;
        }
        if (opts.mode == engine::vm) {
            auto cmplr = compiler::create_with_state(&consts, symbols);
            cmplr.compile(prgrm);
            auto machine = vm::create_with_state(cmplr.byte_code(), &globals);
            machine.run();
            const auto* result = machine.last_popped();
            if (!result->is(object::object_type::null)) {
                std::cout << result->inspect() << "\n";
            }
        } else {
            const auto* result = prgrm->eval(global_env);
            if (!result->is(object::object_type::null)) {
                std::cout << result->inspect() << "\n";
            }
        }
        if (opts.debug_env) {
            global_env->debug();
        }
        std::cout << prompt;
    }
    return 0;
}
}  // namespace

auto main(int argc, char* argv[]) -> int
{
    auto program = std::string_view(*argv);
    auto opts = parse_command_line(program, argc - 1, ++argv);
    if (opts.help) {
        show_usage(program);
    }
    try {
        if (!opts.file.empty()) {
            return run_file(opts);
        }
        return run_repl(opts);

    } catch (const std::exception& e) {
        monkey_business();
        std::cerr << "Caught an exception: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
