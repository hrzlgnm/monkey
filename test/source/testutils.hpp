#pragma once

#include <utility>

#include <parser/parser.hpp>
#include <parser/program.hpp>

auto assert_no_parse_errors(const parser& prsr) -> bool;
using parsed_program = std::pair<program_ptr, parser>;
auto assert_program(std::string_view input) -> parsed_program;
