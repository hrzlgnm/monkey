#pragma once

#include "node.hpp"
#include "token.hpp"

struct expression : node
{
    using node::node;
    explicit expression(token tokn);
    auto token_literal() const -> std::string_view override;
    token tkn {};
};
using expression_ptr = std::shared_ptr<expression>;
