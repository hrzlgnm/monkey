#include <iostream>
#include <string>

#include "lib.hpp"

auto main() -> int
{
    auto lib = lexxur {"Lexxur"};
    auto le_token = lib.next_token();

    std::cout << "le token: `" << le_token.literal << "`\n";

    return 0;
}
