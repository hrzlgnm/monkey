#include <future>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include "environment.hpp"
#include "environment_fwd.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "statements.hpp"
#include "util.hpp"

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

auto main() -> int
{
    std::cout << prompt;

    auto input = std::string {};
    auto globals = std::make_shared<environment>();
    while (getline(std::cin, input)) {
        auto lxr = lexer {input};
        auto prsr = parser {lxr};
        auto prgrm = prsr.parse_program();
        if (!prsr.errors().empty()) {
            print_parse_errors(prsr.errors());
            continue;
        }
        auto evaluated = prgrm->eval(globals);
        if (!evaluated.is_nil()) {
            std::cout << std::to_string(evaluated.value) << "\n";
        }
        std::cout << prompt;
    }
    // break the cycle
    globals->store.clear();
    return 0;
}
