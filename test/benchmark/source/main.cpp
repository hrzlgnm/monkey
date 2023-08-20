#include <chrono>
#include <iostream>
#include <memory>
#include <span>
#include <string_view>

#include <compiler/compiler.hpp>
#include <compiler/vm.hpp>
#include <eval/environment.hpp>
#include <eval/object.hpp>
#include <fmt/ostream.h>
#include <lexer/lexer.hpp>
#include <parser/parser.hpp>

using namespace std::chrono_literals;

auto main(int argc, char* argv[]) -> int
{
    const char* input = R"(
let fibonacci = fn(x) {
  if (x == 0) {
    0
  } else {
    if (x == 1) {
      return 1;
    } else {
      fibonacci(x - 1) + fibonacci(x - 2);
    }
  }
};
fibonacci(35);
    )";

    auto engine_vm = true;
    for (const std::string_view arg : std::span(++argv, static_cast<std::size_t>(argc - 1))) {
        if (arg == "--eval") {
            engine_vm = false;
        }
    }

    auto lxr = lexer {input};
    auto prsr = parser {lxr};
    auto prgrm = prsr.parse_program();
    object result;
    std::chrono::duration<double> duration {};
    if (engine_vm) {
        auto cmplr = compiler::create();
        cmplr.compile(prgrm);
        auto mchn = vm::create(cmplr.byte_code());
        auto start = std::chrono::steady_clock::now();
        mchn.run();
        result = mchn.last_popped();
        auto end = std::chrono::steady_clock::now();
        duration = end - start;
    } else {
        auto env = std::make_shared<environment>();
        auto start = std::chrono::steady_clock::now();
        result = prgrm->eval(env);
        auto end = std::chrono::steady_clock::now();
        duration = end - start;
    }
    fmt::print("engine={}, result={}, duration={}\n",
               (engine_vm ? "vm" : "eval"),
               std::to_string(result.value),
               duration.count());
    return 0;
}
