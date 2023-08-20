#pragma once

#include <algorithm>
#include <utility>
#include <vector>

#include <ast/program.hpp>
#include <parser/parser.hpp>

auto assert_no_parse_errors(const parser& prsr) -> bool;
using parsed_program = std::pair<program_ptr, parser>;
auto assert_program(std::string_view input) -> parsed_program;

template<typename T>
auto maker(std::initializer_list<T> list) -> std::vector<T>
{
    return std::vector<T> {list};
}

template<typename T>
auto flatten(const std::vector<std::vector<T>>& arrs) -> std::vector<T>
{
    std::vector<T> result;
    for (const auto& arr : arrs) {
        std::copy(arr.cbegin(), arr.cend(), std::back_inserter(result));
    }
    return result;
}
