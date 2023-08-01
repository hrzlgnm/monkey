#include "testutils.hpp"

#include <gtest/gtest.h>

#include "parser.hpp"

auto assert_no_parse_errors(const parser& prsr) -> void
{
    EXPECT_TRUE(prsr.errors().empty()) << "expected no errors, got: " << testing::PrintToString(prsr.errors());
}
