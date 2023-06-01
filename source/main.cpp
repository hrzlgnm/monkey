#include <future>
#include <iostream>
#include <string>

#include "lib.hpp"

constexpr auto prompt = ">> ";

auto main() -> int
{
    std::cout << prompt;

    auto input = std::string {};
    while (getline(std::cin, input)) {
        auto lxr = lexer {input};
        auto tok = lxr.next_token();
        while (tok.type != token_type::eof) {
            std::cout << tok << '\n';
            tok = lxr.next_token();
        }
        std::cout << prompt;
    }

    return 0;
}
