#include <exception>
#include <iostream>
#include <iterator>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <ast/builtin_function_expression.hpp>
#include <compiler/compiler.hpp>
#include <compiler/vm.hpp>
#include <eval/environment.hpp>
#include <lexer/lexer.hpp>
#include <parser/parser.hpp>

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
    bool debug_env {};
    run_mode mode {};
    std::string_view file;
};

auto parse_command_line(int argc, char** argv) -> cmdline_options
{
    cmdline_options opts {};
    for (std::string_view arg : std::span(argv, static_cast<size_t>(argc))) {
        if (arg.at(0) == '-' && arg.size() == 1) {
            fmt::print("WARNING: skipping unknown command line arg {}\n", arg);
            continue;
        }
        if (arg.at(0) == '-' && arg.size() > 1) {
            switch (arg.at(1)) {
                case 'i':
                    opts.mode = run_mode::interpret;
                    break;
                case 'h':
                    opts.help = true;
                    break;
                case 'd':
                    opts.debug_env = true;
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

auto show_usage(std::string_view program, std::string_view error_msg = {})
{
    if (!error_msg.empty()) {
        fmt::print("Error: {}\n", error_msg);
    }
    fmt::print("Usage: {} [-d] [-i] [-h] [<file>]\n\n", program);
}

auto main(int argc, char* argv[]) -> int
{
    auto program = std::string_view(*argv);
    auto opts = parse_command_line(argc - 1, ++argv);
    if (opts.help) {
        show_usage(program);
        return 0;
    }
    if (!opts.file.empty()) {
        show_usage(program, "file is not supported yet, sorry");
        return 1;
    }
    auto input = std::string {};
    std::cout << prompt;
    auto global_env = std::make_shared<environment>();
    auto symbols = symbol_table::create();

    for (auto idx = 0UL; const auto& builtin : builtin_function_expression::builtins) {
        global_env->set(builtin.name, object {bound_function(&builtin, environment_ptr {})});
        symbols->define_builtin(idx, builtin.name);
        idx++;
    }
    auto consts = std::make_shared<constants>();
    auto globals = std::make_shared<constants>(globals_size);

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
                auto cmplr = compiler::create_with_state(consts, symbols);
                cmplr.compile(prgrm);
                auto machine = vm::create_with_state(cmplr.byte_code(), globals);
                machine.run();
                auto result = machine.last_popped();
                std::cout << std::to_string(result.value) << "\n";
            } else {
                auto evaluated = prgrm->eval(global_env);
                if (!evaluated.is_nil()) {
                    std::cout << std::to_string(evaluated.value) << "\n";
                }
            }
            std::move(prgrm->statements.begin(), prgrm->statements.end(), std::back_inserter(cache));
            if (opts.debug_env) {
                debug_env(global_env);
            }
            std::cout << prompt;
        }
        global_env->break_cycle();
    } catch (const std::exception& e) {
        monkey_business();
        std::cerr << "Caught an exception: " << e.what() << "\n";
    }
    return 0;
}
