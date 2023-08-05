#include "testutils.hpp"

#include <fmt/format.h>
#include <gtest/gtest.h>

#include "parser.hpp"

auto assert_no_parse_errors(const parser& prsr) -> bool
{
    EXPECT_TRUE(prsr.errors().empty()) << "expected no errors, got: "
                                       << fmt::format("{}", fmt::join(prsr.errors(), ", "));
    return !prsr.errors().empty();
}
