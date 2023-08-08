#include "testutils.hpp"

#include <fmt/format.h>
#include <gtest/gtest.h>

auto assert_no_parse_errors(const parser& prsr) -> bool
{
    EXPECT_TRUE(prsr.errors().empty()) << "expected no errors, got: "
                                       << fmt::format("{}", fmt::join(prsr.errors(), ", "));
    return !prsr.errors().empty();
}

auto assert_program(std::string_view input) -> parsed_program
{
    auto prsr = parser {lexer {input}};
    auto prgrm = prsr.parse_program();
    if (assert_no_parse_errors(prsr)) {
        std::cerr << "while parsing: `" << input << "`";
    };
    return {std::move(prgrm), std::move(prsr)};
}
