#include <future>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include "builtin_function_expression.hpp"
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

auto print_parse_errors(const std::vector<std::string>& errors)
{
    std::cerr << monkey_face;
    std::cerr << "Woops! We ran into some monkey business here!\n";
    std::cerr << "  parser errorrs: \n";
    for (const auto& error : errors) {
        std::cerr << "\t" << error << "\n";
    }
}
// TODO: handle exceptions
auto main() -> int
{
    std::cout << prompt;

    auto input = std::string {};
    auto globals = std::make_shared<environment>();
    for (const auto& builtin : builtin_function_expression::builtins) {
        globals->set(builtin.name, object {bound_function(&builtin, environment_ptr {})});
    }
    auto statements = std::vector<statement_ptr>();
    while (getline(std::cin, input)) {
        auto lxr = lexer {input};
        auto prsr = parser {lxr};
        auto prgrm = prsr.parse_program();
        if (!prsr.errors().empty()) {
            print_parse_errors(prsr.errors());
            continue;
        }
        compiler piler;
        piler.compile(prgrm);
        auto bytecode = piler.code();
        vm machine {
            .consts = bytecode.consts,
            .code = bytecode.code,
        };
        machine.run();
        auto stack_top = machine.stack_top();
        std::cout << std::to_string(stack_top.value) << "\n";

        // TODO: add switch to compile / evaluated
        /*
                auto evaluated = prgrm->eval(globals);
                std::move(prgrm->statements.begin(), prgrm->statements.end(), std::back_inserter(statements));
                if (!evaluated.is_nil()) {
                    std::cout << std::to_string(evaluated.value) << "\n";
                }
                */
        std::cout << prompt;
    }
    globals->break_cycle();
    return 0;
}
