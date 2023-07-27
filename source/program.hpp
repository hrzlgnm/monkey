#pragma once

#include <vector>

#include "node.hpp"
#include "statements.hpp"

struct program : node
{
    using node::node;
    auto token_literal() const -> std::string_view override;
    auto string() const -> std::string override;
    auto eval(environment_ptr env) const -> object override;

    std::vector<statement_ptr> statements {};
};
using program_ptr = std::unique_ptr<program>;
