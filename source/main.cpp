#include <exception>
#include <future>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "builtin_function_expression.hpp"
#include "code.hpp"
#include "compiler.hpp"
#include "environment.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "util.hpp"
#include "vm.hpp"

constexpr auto prompt = ">> ";

constexpr auto monkey_face = R"r(
             __,__
     .--. .-"     "-. .--.
    / .. \/ .-. .-. \/ .. \
   | |  '| /   Y   \ |'  | |
   | \   \ \ 0 | 0 / /  /  |
    \ '-,\.-"""""""-./,-' /
     ''-' /_  ^ ^ _ \ '-''
         | \._   _./ |
         \  \ '~' / /
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
    std::cerr << "  parser errorrs: \n";
    for (const auto& error : errors) {
        std::cerr << "\t" << error << "\n";
    }
}

enum class run_mode
{
    compile,
    interpret,
};

struct cmdline_options
{
    bool help {};
    run_mode mode {};
    std::string_view file;
};

auto parse_command_line(std::vector<std::string_view>&& args) -> cmdline_options
{
    cmdline_options opts {};
    for (const auto& arg : args) {
        if (arg[0] == '-') {
            switch (arg[1]) {
                case 'i':
                    opts.mode = run_mode::interpret;
                    break;
                case 'h':
                    opts.help = true;
                    break;
            }
        } else {
            if (opts.file.empty()) {
                opts.file = arg;
            }
        }
    }
    return opts;
}
auto show_usage(std::string_view program, std::string_view message = {})
{
    if (!message.empty()) {
        fmt::print("Error: {}\n", message);
    }
    fmt::print("Usage: {} [-i] [-h] [<file>]\n\n", program);
}

auto main(int argc, char** argv) -> int
{
    auto args = std::vector<std::string_view>();
    std::transform(
        argv + 1, argv + argc, std::back_inserter(args), [](const char* arg) { return std::string_view(arg); });
    auto opts = parse_command_line(std::move(args));
    if (opts.help) {
        show_usage(*argv);
        return 0;
    }
    if (!opts.file.empty()) {
        show_usage(*argv, "file is not supported yet, sorry");
        return 1;
    }
    auto input = std::string {};
    std::cout << prompt;
    auto global_env = std::make_shared<environment>();
    for (const auto& builtin : builtin_function_expression::builtins) {
        global_env->set(builtin.name, object {bound_function(&builtin, environment_ptr {})});
    }
    auto consts = std::make_shared<constants>();
    auto globals = std::make_shared<constants>(globals_size);
    auto symbols = std::make_shared<symbol_table>();
    try {
        auto cache = std::vector<statement_ptr>();
        while (getline(std::cin, input)) {
            auto lxr = lexer {input};
            auto prsr = parser {lxr};
            auto prgrm = prsr.parse_program();
            if (!prsr.errors().empty()) {
                print_parse_errors(prsr.errors());
                continue;
            }
            if (opts.mode == run_mode::compile) {
                auto cmplr = make_compiler_with_state(consts, symbols);
                cmplr.compile(prgrm);
                auto machine = make_vm_with_state(std::move(cmplr.code), globals);
                machine.run();
                auto stack_top = machine.last_popped();
                std::cout << std::to_string(stack_top.value) << "\n";
            } else {
                auto evaluated = prgrm->eval(global_env);
                if (!evaluated.is_nil()) {
                    std::cout << std::to_string(evaluated.value) << "\n";
                }
            }
            std::move(prgrm->statements.begin(), prgrm->statements.end(), std::back_inserter(cache));
            std::cout << prompt;
        }
        global_env->break_cycle();
    } catch (const std::exception& e) {
        monkey_business();
        std::cerr << "Caught an exception: " << e.what() << "\n";
    }
    return 0;
}
